#pragma once

#include "AppearanceData.h"
#include "Log.h"

namespace CurrentFashionSetter
{
    bool CallProcessEvent(SDK::UObject* object, SDK::UFunction* function, void* params, std::ofstream& log) = delete;
    bool IsPlayerAppearanceObject(const SDK::UObject* object);
    SDK::UHTPlayerAppearance* AsPlayerAppearance(SDK::UObject* object, std::ofstream& log);

    std::string Hex32(DWORD value);

    template<typename ParamsType>
    DWORD InvokeProcessEventSeh(SDK::UObject* object, SDK::UFunction* function, ParamsType* params, bool* called)
    {
        DWORD exceptionCode = 0;
        *called = false;

        __try
        {
            __try
            {
                object->ProcessEvent(function, params);
                *called = true;
            }
            __finally
            {
            }
        }
        __except (exceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
        {
        }

        return exceptionCode;
    }

    template<typename ParamsType>
    bool CallProcessEvent(SDK::UObject* object, SDK::UFunction* function, ParamsType* params, std::ofstream& log)
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

        bool called = false;
        const DWORD exceptionCode = InvokeProcessEventSeh(object, function, params, &called);

        function->FunctionFlags = flags;

        if (exceptionCode != 0)
        {
            LogLine(log, "CallProcessEvent SEH exception=" + Hex32(exceptionCode)
                + " function=" + function->GetName()
                + " object=" + ObjectName(object));
            return false;
        }

        if (called)
        {
            LogLine(log, "called " + function->GetName());
        }

        return called;
    }

    template<typename SoftType>
    SDK::TSoftObjectPtr<SDK::UObject> CopySoftObjectPtr(const SoftType& source)
    {
        SDK::TSoftObjectPtr<SDK::UObject> result = {};
        static_assert(sizeof(result) == sizeof(source));
        memcpy(&result, &source, sizeof(result));
        return result;
    }

    template<typename SoftType>
    std::string SoftObjectPathToString(const SoftType& source)
    {
        return source.ObjectID.AssetPath.PackageName.ToString()
            + "."
            + source.ObjectID.AssetPath.AssetName.ToString();
    }
}
