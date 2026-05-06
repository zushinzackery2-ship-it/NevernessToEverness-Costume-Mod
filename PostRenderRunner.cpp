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
    SDK::FName g_LastFashionId;

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

        bool IsAntiFadeIntact(SDK::AHTPlayerCameraManager* cm)
        {
            if (cm == nullptr)
            {
                return false;
            }

            return cm->RunSettings.PlayerFadeDistance.TargetValue == 0.0f
                && cm->RunSettings.PlayerHideDistance.TargetValue == 0.0f;
        }

        void RunAntiFadeOnce()
        {
            const char* State = nullptr;
            const AntiFadeMod::FLocalContext Context = AntiFadeMod::ResolveLocalContext(&State);
            if (Context.CameraManager == nullptr)
            {
                return;
            }

            __try
            {
                AntiFadeMod::FPatchStats Stats = AntiFadeMod::PatchAllCameraManagers(Context.CameraManager);
                AntiFadeMod::ApplySelfSettingCamera(Context.CameraManager, Stats);
                AntiFadeMod::RestoreOpacityOnce(Context.Character, Stats);

                if (Stats.CameraManagers > 0 || Stats.SelfSettingCalls > 0)
                {
                    AntiFadeMod::PrintStats(Context.CameraManager, Context.Character, Stats);
                    WriteRawLogLine("[AntiFadeMod] patch applied");
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                WriteRawLogLine("[AntiFadeMod] patch failed with SEH, giving up");
            }
        }

        void MaintainMods()
        {
            const AntiFadeMod::FLocalContext ctx = AntiFadeMod::ResolveLocalContext(nullptr);

            if (ctx.CameraManager != nullptr)
            {
                if (!g_AntiFadeEverApplied)
                {
                    RunAntiFadeOnce();
                    g_AntiFadeEverApplied = true;
                }
                else if (!IsAntiFadeIntact(ctx.CameraManager))
                {
                    WriteRawLogLine("[AntiFadeMod] drift detected, re-patching");
                    RunAntiFadeOnce();
                }
            }

            if (!g_LastFashionId.IsNone() && ctx.Character != nullptr)
            {
                auto* playerChar = static_cast<SDK::AHTPlayerCharacter*>(ctx.Character);
                if (IsValidObject(reinterpret_cast<SDK::UObject*>(playerChar)))
                {
                    if (playerChar->FashionID != g_LastFashionId)
                    {
                        WriteRawLogLine("[FashionDrift] drift detected, re-applying");
                        ApplyFashionById(g_LastFashionId);
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