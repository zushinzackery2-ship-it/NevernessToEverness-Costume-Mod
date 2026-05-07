#include "Pch.h"

#include "PostRenderRunner.h"

#include "FashionSetter.h"
#include "AppearanceData.h"
#include "Log.h"
#include "AntiFadeMod/CameraPatch.hpp"
#include "AntiFadeMod/SdkHelpers.hpp"
#include "SDK/PostRenderHook.hpp"

namespace CurrentFashionSetter
{
    volatile bool g_AntiFadeEverApplied = false;
    SDK::APlayerController* g_LastAntiFadePlayerController = nullptr;
    SDK::AHTPlayerCameraManager* g_LastAntiFadeCameraManager = nullptr;
    std::unordered_map<std::string, SDK::FName> g_LastFashionByCharacter;

    namespace
    {
        constexpr int kUpDirection = -1;
        constexpr int kDownDirection = 1;
        constexpr ULONGLONG kMaintainModsIntervalMs = 300;

        volatile LONG g_PendingDirection = 0;
        ULONGLONG g_LastMaintainModsTick = 0;

        struct ScopedSdkExecutorDepth
        {
            ScopedSdkExecutorDepth()
            {
                ++SDK::InSDKUtils::GameThreadExecutorDepthStorage();
            }

            ~ScopedSdkExecutorDepth()
            {
                --SDK::InSDKUtils::GameThreadExecutorDepthStorage();
            }
        };

        struct KeyMonitorContext
        {
            volatile LONG Busy = 0;
        };

        void LogSdkMessage(const char* text, void*)
        {
            WriteRawLogLine(text);
        }

        constexpr DWORD kMsvcCppExceptionCode = 0xE06D7363;

        int CaptureSwapException(DWORD exceptionCode, DWORD* capturedExceptionCode)
        {
            if (capturedExceptionCode != nullptr)
            {
                *capturedExceptionCode = exceptionCode;
            }

            if (exceptionCode == kMsvcCppExceptionCode)
            {
                return EXCEPTION_CONTINUE_SEARCH;
            }

            return EXCEPTION_EXECUTE_HANDLER;
        }

        DWORD RunSwapSehGuarded(int direction)
        {
            DWORD exceptionCode = ERROR_SUCCESS;
            __try
            {
                RunFashionSwapOnGameThread(direction);
            }
            __except (CaptureSwapException(GetExceptionCode(), &exceptionCode))
            {
            }

            return exceptionCode;
        }

        void RunSwapGuarded(int direction, KeyMonitorContext* context)
        {
            DWORD sehExceptionCode = ERROR_SUCCESS;
            bool cxxExceptionCaught = false;

            try
            {
                ScopedSdkExecutorDepth executorDepth;
                sehExceptionCode = RunSwapSehGuarded(direction);
            }
            catch (...)
            {
                cxxExceptionCaught = true;
            }

            if (sehExceptionCode != ERROR_SUCCESS)
            {
                char buffer[128] = {};
                sprintf_s(buffer, "[CurrentFashionSetter] game-thread SEH exception=0x%08X", sehExceptionCode);
                WriteRawLogLine(buffer);
            }

            if (cxxExceptionCaught)
            {
                WriteRawLogLine("[CurrentFashionSetter] game-thread C++ exception caught during fashion swap");
            }

            InterlockedExchange(&context->Busy, 0);
        }

        bool IsAntiFadeIntactGuarded(SDK::AHTPlayerCameraManager* cameraManager)
        {
            if (cameraManager == nullptr)
            {
                return false;
            }

            DWORD exceptionCode = ERROR_SUCCESS;
            bool intact = false;
            __try
            {
                intact = AntiFadeMod::IsCameraManagerAntiFadeIntact(cameraManager);
            }
            __except (exceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
            {
            }

            if (exceptionCode != ERROR_SUCCESS)
            {
                char buffer[128] = {};
                sprintf_s(buffer, "[AntiFadeMod] integrity check SEH exception=0x%08X", exceptionCode);
                WriteRawLogLine(buffer);
            }

            return intact;
        }

        bool RunAntiFadeOnce(const AntiFadeMod::FLocalContext& Context)
        {
            if (Context.CameraManager == nullptr)
            {
                return false;
            }

            bool Applied = false;
            __try
            {
                AntiFadeMod::FPatchStats Stats = AntiFadeMod::PatchCurrentCameraManager(Context.CameraManager);
                AntiFadeMod::RestoreOpacityOnce(Context.Character, Stats);

                if (Stats.CameraManagers > 0)
                {
                    AntiFadeMod::PrintStats(Context.CameraManager, Context.Character, Stats);
                    WriteRawLogLine("[AntiFadeMod] patch applied");
                }

                Applied = IsAntiFadeIntactGuarded(Context.CameraManager);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                WriteRawLogLine("[AntiFadeMod] patch failed with SEH");
            }

            return Applied;
        }

        void MarkAntiFadeContext(const AntiFadeMod::FLocalContext& Context)
        {
            g_AntiFadeEverApplied = true;
            g_LastAntiFadePlayerController = Context.PlayerController;
            g_LastAntiFadeCameraManager = Context.CameraManager;
        }

        void MaintainMods()
        {
            const AntiFadeMod::FLocalContext ctx = AntiFadeMod::ResolveLocalContext(nullptr);

            if (ctx.CameraManager != nullptr)
            {
                const bool cameraContextChanged = ctx.CameraManager != g_LastAntiFadeCameraManager
                    || ctx.PlayerController != g_LastAntiFadePlayerController;

                if (!g_AntiFadeEverApplied || cameraContextChanged)
                {
                    if (cameraContextChanged && g_LastAntiFadeCameraManager != nullptr)
                    {
                        WriteRawLogLine("[AntiFadeMod] camera context changed, re-patching");
                    }

                    if (RunAntiFadeOnce(ctx))
                    {
                        MarkAntiFadeContext(ctx);
                    }
                }
                else if (!IsAntiFadeIntactGuarded(ctx.CameraManager))
                {
                    WriteRawLogLine("[AntiFadeMod] drift detected, re-patching");
                    if (RunAntiFadeOnce(ctx))
                    {
                        MarkAntiFadeContext(ctx);
                    }
                }
            }
            else if (g_LastAntiFadeCameraManager != nullptr)
            {
                WriteRawLogLine("[AntiFadeMod] camera context lost");
                g_AntiFadeEverApplied = false;
                g_LastAntiFadePlayerController = nullptr;
                g_LastAntiFadeCameraManager = nullptr;
            }

            if (ctx.Character != nullptr)
            {
                auto* playerChar = static_cast<SDK::AHTPlayerCharacter*>(ctx.Character);
                if (IsValidObject(reinterpret_cast<SDK::UObject*>(playerChar)))
                {
                    const std::string characterIdStr = playerChar->DefaultCharacterID.ToString();
                    auto it = g_LastFashionByCharacter.find(characterIdStr);
                    if (it != g_LastFashionByCharacter.end() && playerChar->FashionID != it->second)
                    {
                        WriteRawLogLine("[FashionDrift] drift detected, re-applying");
                        ApplyFashionById(it->second);
                    }
                }
            }
        }

        void CALLBACK PostRenderCallback(SDK::UGameViewportClient*, SDK::UCanvas*, void* context)
        {
            const ULONGLONG now = GetTickCount64();
            if (now - g_LastMaintainModsTick >= kMaintainModsIntervalMs)
            {
                g_LastMaintainModsTick = now;
                MaintainMods();
            }

            auto* monitor = static_cast<KeyMonitorContext*>(context);
            if (monitor == nullptr)
            {
                return;
            }

            const LONG direction = InterlockedExchange(&g_PendingDirection, 0);
            if (direction == 0)
            {
                return;
            }

            if (InterlockedCompareExchange(&monitor->Busy, 1, 0) != 0)
            {
                WriteRawLogLine("[CurrentFashionSetter] hotkey ignored: previous fashion swap still running");
                return;
            }

            WriteRawLogLine(direction < 0
                ? "[CurrentFashionSetter] VK_NUMPAD8 pressed: switch previous fashion"
                : "[CurrentFashionSetter] VK_NUMPAD2 pressed: switch next fashion");
            RunSwapGuarded(direction, monitor);
        }
    }

    void NotifyKeyPress(int direction)
    {
        InterlockedExchange(&g_PendingDirection, static_cast<LONG>(direction));
    }

    DWORD RegisterFashionHotkeyMonitor()
    {
        static KeyMonitorContext context = {};
        static SDK::PostRenderHook::RegistrationHandle handle = 0;

        if (handle != 0)
        {
            WriteRawLogLine("[CurrentFashionSetter] fashion hotkey monitor already registered");
            return ERROR_SUCCESS;
        }

        handle = SDK::PostRenderHook::RegisterPostRenderCallback(
            &PostRenderCallback,
            &context,
            &LogSdkMessage);
        if (handle == 0)
        {
            WriteRawLogLine("[CurrentFashionSetter] RegisterPostRenderCallback failed for hotkey monitor");
            return ERROR_INVALID_FUNCTION;
        }

        WriteRawLogLine("[CurrentFashionSetter] fashion hotkey monitor registered: NUMPAD8 previous, NUMPAD2 next");

        return ERROR_SUCCESS;
    }
}