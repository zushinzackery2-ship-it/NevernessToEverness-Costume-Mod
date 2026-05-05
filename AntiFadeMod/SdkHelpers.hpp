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
bool CallProcessEvent(SDK::UObject* Object, SDK::UFunction* Function, void* Params);
}

