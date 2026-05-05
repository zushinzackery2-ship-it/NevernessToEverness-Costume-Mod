#include "Pch.h"

#include "PostRenderRunner.h"

#include "FashionSetter.h"
#include "Log.h"
#include "AntiFadeMod/CameraPatch.hpp"
#include "AntiFadeMod/SdkHelpers.hpp"
#include "SDK/PostRenderHook.hpp"

namespace CurrentFashionSetter
{
    namespace
    {
        constexpr int kUpDirection = -1;
        constexpr int kDownDirection = 1;

        volatile LONG g_PendingDirection = 0;

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

        void CALLBACK KeyMonitorPostRenderCallback(SDK::UGameViewportClient*, SDK::UCanvas*, void* context)
        {
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

        SDK::PostRenderHook::RegistrationHandle g_AntiFadeHandle = 0;

        void CALLBACK AntiFadePostRenderCallback(SDK::UGameViewportClient*, SDK::UCanvas*, void*)
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
                    WriteRawLogLine("[AntiFadeMod] patch applied, unregistering callback");
                }
                else
                {
                    return;
                }
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return;
            }

            SDK::PostRenderHook::UnregisterPostRenderCallback(g_AntiFadeHandle);
            g_AntiFadeHandle = 0;
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
            &KeyMonitorPostRenderCallback,
            &context,
            &LogSdkMessage);
        if (handle == 0)
        {
            WriteRawLogLine("[CurrentFashionSetter] RegisterPostRenderCallback failed for hotkey monitor");
            return ERROR_INVALID_FUNCTION;
        }

        WriteRawLogLine("[CurrentFashionSetter] fashion hotkey monitor registered: NUMPAD8 previous, NUMPAD2 next");

        g_AntiFadeHandle = SDK::PostRenderHook::RegisterPostRenderCallback(
            &AntiFadePostRenderCallback, nullptr, &LogSdkMessage, nullptr);
        if (g_AntiFadeHandle == 0)
        {
            WriteRawLogLine("[AntiFadeMod] RegisterPostRenderCallback failed");
        }
        else
        {
            WriteRawLogLine("[AntiFadeMod] PostRender callback registered, waiting for camera manager...");
        }

        return ERROR_SUCCESS;
    }
}