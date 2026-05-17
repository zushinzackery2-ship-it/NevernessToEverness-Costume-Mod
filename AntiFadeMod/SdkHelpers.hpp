#pragma once

#include "../Log.h"

namespace AntiFadeMod
{
struct FLocalContext
{
    SDK::APlayerController* PlayerController = nullptr;
    SDK::AHTPlayerCameraManager* CameraManager = nullptr;
    SDK::AHTAbilityCharacter* Character = nullptr;
};

SDK::UClass* FindClassByName(const char* ClassName);
bool IsClassOrChildOf(const SDK::UClass* TypeClass, const SDK::UClass* BaseClass);
FLocalContext ResolveLocalContext(const char** State);
SDK::UFunction* FindFunctionByName(const SDK::UClass* ObjectClass, const char* ClassName, const char* FuncName);
bool CallProcessEvent(SDK::UObject* Object, SDK::UFunction* Function, void* Params) = delete;

template<typename ParamsType>
bool CallProcessEvent(SDK::UObject* Object, SDK::UFunction* Function, ParamsType* Params)
{
    if (Object == nullptr || Function == nullptr)
    {
        return false;
    }

#if defined(SDK_ENABLE_GAME_THREAD_EXECUTOR)
    if (!SDK::InSDKUtils::HasGameThreadExecutor()
        && SDK::InSDKUtils::GameThreadExecutorDepthStorage() <= 0)
    {
        CurrentFashionSetter::WriteRawLogLine("[AntiFadeMod] CallProcessEvent blocked outside SDK game-thread executor context");
        return false;
    }
#endif

    const uint32_t OriginalFlags = Function->FunctionFlags;
    DWORD ExceptionCode = ERROR_SUCCESS;
    bool Called = false;

    __try
    {
        __try
        {
            Function->FunctionFlags |= 0x400;
            Object->ProcessEvent(Function, Params);
            Called = true;
        }
        __except ((ExceptionCode = GetExceptionCode()) == 0xE06D7363
            ? EXCEPTION_CONTINUE_SEARCH
            : EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
    __finally
    {
        Function->FunctionFlags = OriginalFlags;
    }

    if (ExceptionCode != ERROR_SUCCESS)
    {
        char Buffer[256] = {};
        sprintf_s(Buffer,
            "[AntiFadeMod] CallProcessEvent SEH exception=0x%08X function=%p object=%p",
            ExceptionCode,
            static_cast<void*>(Function),
            static_cast<void*>(Object));
        CurrentFashionSetter::WriteRawLogLine(Buffer);
    }

    return Called;
}
}

