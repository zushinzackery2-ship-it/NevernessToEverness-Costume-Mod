#include "Pch.h"

#include "Log.h"

namespace CurrentFashionSetter
{
    HANDLE gConsoleOut = INVALID_HANDLE_VALUE;
    bool gConsoleInitialized = false;

    BOOL WINAPI ConsoleCtrlHandler(DWORD)
    {
        return TRUE;
    }

    void DisableConsoleCloseButton()
    {
        HWND window = GetConsoleWindow();
        if (window == nullptr)
        {
            return;
        }

        HMENU menu = GetSystemMenu(window, FALSE);
        if (menu == nullptr)
        {
            return;
        }

        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
        DrawMenuBar(window);
    }

    void WriteConsoleLine(const char* text)
    {
        if (gConsoleOut == INVALID_HANDLE_VALUE || gConsoleOut == nullptr || text == nullptr)
        {
            return;
        }

        DWORD written = 0;
        WriteFile(gConsoleOut, text, static_cast<DWORD>(strlen(text)), &written, nullptr);
        WriteFile(gConsoleOut, "\r\n", 2, &written, nullptr);
    }

    void InitializeDebugConsole()
    {
        if (gConsoleInitialized)
        {
            return;
        }

        if (!AllocConsole() && GetLastError() != ERROR_ACCESS_DENIED)
        {
            return;
        }

        SetConsoleTitleW(L"Rei CurrentFashionSetter");
        gConsoleOut = CreateFileW(L"CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (gConsoleOut != INVALID_HANDLE_VALUE)
        {
            SetStdHandle(STD_OUTPUT_HANDLE, gConsoleOut);
            SetStdHandle(STD_ERROR_HANDLE, gConsoleOut);
        }

        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
        DisableConsoleCloseButton();
        gConsoleInitialized = true;
    }

    void ResetLogFile()
    {
        CreateDirectoryW(L"C:\\Rei-Dumper", nullptr);
        DeleteFileW(kLogPath);
    }

    void WriteRawLogLine(const char* text)
    {
        CreateDirectoryW(L"C:\\Rei-Dumper", nullptr);
        HANDLE file = CreateFileW(kLogPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD written = 0;
        WriteFile(file, text, static_cast<DWORD>(strlen(text)), &written, nullptr);
        WriteFile(file, "\r\n", 2, &written, nullptr);
        CloseHandle(file);
        WriteConsoleLine(text);
    }

    void WriteRawLogLine(const std::string& text)
    {
        WriteRawLogLine(text.c_str());
    }

    std::ofstream OpenAppendLog()
    {
        CreateDirectoryW(L"C:\\Rei-Dumper", nullptr);
        return std::ofstream(kLogPath, std::ios::out | std::ios::app);
    }

    void LogLine(std::ofstream& log, const std::string& text)
    {
        std::string line = "[CurrentFashionSetter] " + text;
        log << line << "\n";
        log.flush();
        WriteConsoleLine(line.c_str());
    }

    std::string PointerString(const void* value)
    {
        char buffer[32] = {};
        sprintf_s(buffer, "0x%p", value);
        return buffer;
    }
}
