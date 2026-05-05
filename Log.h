#pragma once

namespace CurrentFashionSetter
{
    constexpr const wchar_t* kLogPath = L"C:\\Rei-Dumper\\CurrentFashionSetter.log";

    void InitializeDebugConsole();
    void ResetLogFile();
    void WriteRawLogLine(const char* text);
    void WriteRawLogLine(const std::string& text);
    std::ofstream OpenAppendLog();
    void LogLine(std::ofstream& log, const std::string& text);
    std::string PointerString(const void* value);
}
