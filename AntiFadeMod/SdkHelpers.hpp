#pragma once

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

    const uint32_t OriginalFlags = Function->FunctionFlags;
    bool Called = false;

    __try
    {
        Function->FunctionFlags |= 0x400;
        Object->ProcessEvent(Function, Params);
        Called = true;
    }
    __finally
    {
        Function->FunctionFlags = OriginalFlags;
    }

    return Called;
}
}

