#include "Pch.h"

#include "Log.h"
#include "PostRenderRunner.h"

namespace CurrentFashionSetter
{
    namespace
    {
        LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
        {
            if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
            {
                const auto* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
                if (kb->vkCode == VK_NUMPAD8)
                {
                    NotifyKeyPress(-1);
                }
                else if (kb->vkCode == VK_NUMPAD2)
                {
                    NotifyKeyPress(1);
                }
            }

            return CallNextHookEx(nullptr, nCode, wParam, lParam);
        }

        void RunKeyboardHookMessagePump()
        {
            HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
            if (hook == nullptr)
            {
                char buffer[128] = {};
                sprintf_s(buffer, "[CurrentFashionSetter] WH_KEYBOARD_LL install failed error=%lu", GetLastError());
                WriteRawLogLine(buffer);
                return;
            }

            WriteRawLogLine("[CurrentFashionSetter] WH_KEYBOARD_LL hook installed; message pump running");

            MSG msg = {};
            while (GetMessageW(&msg, nullptr, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            UnhookWindowsHookEx(hook);
            WriteRawLogLine("[CurrentFashionSetter] WH_KEYBOARD_LL hook removed");
        }
    }

    DWORD WINAPI SetterThread(void*)
    {
        InitializeDebugConsole();
        ResetLogFile();
        WriteRawLogLine("[CurrentFashionSetter] start");
        WriteRawLogLine("[CurrentFashionSetter] console online; close button disabled to protect host process");
        WriteRawLogLine("[CurrentFashionSetter] log path: C:\\Rei-Dumper\\CurrentFashionSetter.log");

        __try
        {
            const DWORD result = RegisterFashionHotkeyMonitor();
            if (result != ERROR_SUCCESS)
            {
                char buffer[128] = {};
                sprintf_s(buffer, "[CurrentFashionSetter] hotkey monitor registration failed=%lu", result);
                WriteRawLogLine(buffer);
            }
            else
            {
                RunKeyboardHookMessagePump();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            char buffer[128] = {};
            sprintf_s(buffer, "[CurrentFashionSetter] worker SEH exception=0x%08X", GetExceptionCode());
            WriteRawLogLine(buffer);
        }

        WriteRawLogLine("[CurrentFashionSetter] worker leave; console intentionally kept attached");
        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, CurrentFashionSetter::SetterThread, nullptr, 0, nullptr);
        if (thread != nullptr)
        {
            CloseHandle(thread);
        }
    }

    return TRUE;
}
