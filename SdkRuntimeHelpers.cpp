#include "Pch.h"

#include "SdkRuntimeHelpers.h"

#include "Log.h"

namespace CurrentFashionSetter
{
    std::string Hex32(DWORD value)
    {
        char buffer[16] = {};
        sprintf_s(buffer, "0x%08X", value);
        return buffer;
    }

    DWORD InvokeProcessEventSeh(SDK::UObject* object, SDK::UFunction* function, void* params)
    {
        DWORD exceptionCode = 0;

        __try
        {
            object->ProcessEvent(function, params);
        }
        __except (exceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
        {
        }

        return exceptionCode;
    }

    bool CallProcessEvent(SDK::UObject* object, SDK::UFunction* function, void* params, std::ofstream& log)
    {
        if (!IsValidObject(object) || function == nullptr)
        {
            LogLine(log, "CallProcessEvent target/function invalid object=" + PointerString(object)
                + " function=" + PointerString(function));
            return false;
        }

#if defined(SDK_ENABLE_GAME_THREAD_EXECUTOR)
        if (!SDK::InSDKUtils::HasGameThreadExecutor()
            && SDK::InSDKUtils::GameThreadExecutorDepthStorage() <= 0)
        {
            LogLine(log, "CallProcessEvent blocked outside SDK game-thread executor context function="
                + function->GetName());
            return false;
        }
#endif

        const uint32_t flags = function->FunctionFlags;
        function->FunctionFlags |= 0x400;
        const DWORD exceptionCode = InvokeProcessEventSeh(object, function, params);
        function->FunctionFlags = flags;

        if (exceptionCode != 0)
        {
            LogLine(log, "CallProcessEvent SEH exception=" + Hex32(exceptionCode)
                + " function=" + function->GetName()
                + " object=" + ObjectName(object));
            return false;
        }

        LogLine(log, "called " + function->GetName());
        return true;
    }

    bool IsPlayerAppearanceObject(const SDK::UObject* object)
    {
        if (!IsValidObject(object))
        {
            return false;
        }

        const SDK::UClass* klass = object->Class;
        for (int depth = 0; IsValidObject(klass) && depth < 32; ++depth)
        {
            if (klass->GetName() == "HTPlayerAppearance")
            {
                return true;
            }

            const SDK::UStruct* klassStruct = reinterpret_cast<const SDK::UStruct*>(klass);
            klass = reinterpret_cast<const SDK::UClass*>(klassStruct->Super);
        }

        return false;
    }

    SDK::UHTPlayerAppearance* AsPlayerAppearance(SDK::UObject* object, std::ofstream& log)
    {
        if (!IsPlayerAppearanceObject(object))
        {
            LogLine(log, "object is not UHTPlayerAppearance object=" + ObjectName(object));
            return nullptr;
        }

        return reinterpret_cast<SDK::UHTPlayerAppearance*>(object);
    }
}
