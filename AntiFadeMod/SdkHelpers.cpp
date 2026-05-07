#include "Pch.h"
#include "SdkHelpers.hpp"

namespace AntiFadeMod
{
namespace
{
SDK::AHTAbilityCharacter* ResolveLocalCharacter(SDK::APlayerController* PlayerController)
{
    if (PlayerController == nullptr)
    {
        return nullptr;
    }

    SDK::UClass* AbilityCharacterClass = FindClassByName("HTAbilityCharacter");
    if (AbilityCharacterClass == nullptr)
    {
        return nullptr;
    }

    SDK::APawn* Pawn = PlayerController->AcknowledgedPawn;
    if (Pawn == nullptr)
    {
        Pawn = PlayerController->Pawn;
    }

    if (Pawn == nullptr || Pawn->Class == nullptr)
    {
        return nullptr;
    }

    if (!IsClassOrChildOf(Pawn->Class, AbilityCharacterClass))
    {
        return nullptr;
    }

    return static_cast<SDK::AHTAbilityCharacter*>(Pawn);
}

void SetResolveState(const char** State, const char* Value)
{
    if (State != nullptr)
    {
        *State = Value;
    }
}
}

SDK::UClass* FindClassByName(const char* ClassName)
{
    if (ClassName == nullptr)
    {
        return nullptr;
    }

    return SDK::BasicFilesImpleUtils::FindClassByName(std::string(ClassName), false);
}

bool IsClassOrChildOf(const SDK::UClass* TypeClass, const SDK::UClass* BaseClass)
{
    if (TypeClass == nullptr || BaseClass == nullptr)
    {
        return false;
    }

    for (const SDK::UStruct* ClassNode = TypeClass; ClassNode != nullptr; ClassNode = ClassNode->Super)
    {
        if (ClassNode == BaseClass)
        {
            return true;
        }
    }

    return false;
}

FLocalContext ResolveLocalContext(const char** State)
{
    FLocalContext Context{};

    SDK::UWorld* World = static_cast<SDK::UWorld*>(SDK::InSDKUtils::GetGWorld());
    if (World == nullptr)
    {
        SetResolveState(State, "waiting for GWorld");
        return Context;
    }

    SDK::UGameInstance* GameInstance = World->OwningGameInstance;
    if (GameInstance == nullptr)
    {
        SetResolveState(State, "waiting for GameInstance");
        return Context;
    }

    if (!GameInstance->LocalPlayers.IsValidIndex(0))
    {
        SetResolveState(State, "waiting for LocalPlayer[0]");
        return Context;
    }

    SDK::ULocalPlayer* LocalPlayer = GameInstance->LocalPlayers[0];
    if (LocalPlayer == nullptr)
    {
        SetResolveState(State, "LocalPlayer[0] is null");
        return Context;
    }

    Context.PlayerController = LocalPlayer->PlayerController;
    if (Context.PlayerController == nullptr)
    {
        SetResolveState(State, "waiting for PlayerController");
        return Context;
    }

    SDK::APlayerCameraManager* PlayerCameraManager = Context.PlayerController->PlayerCameraManager;
    if (PlayerCameraManager == nullptr)
    {
        SetResolveState(State, "waiting for HTPlayerCameraManager");
        return Context;
    }

    SDK::UClass* CameraManagerClass = FindClassByName("HTPlayerCameraManager");
    if (PlayerCameraManager->Class == nullptr
        || CameraManagerClass == nullptr
        || !IsClassOrChildOf(PlayerCameraManager->Class, CameraManagerClass))
    {
        SetResolveState(State, "PlayerCameraManager is not HTPlayerCameraManager");
        return Context;
    }

    Context.CameraManager = static_cast<SDK::AHTPlayerCameraManager*>(PlayerCameraManager);
    Context.Character = ResolveLocalCharacter(Context.PlayerController);
    SetResolveState(State, "ready");
    return Context;
}

SDK::UFunction* FindFunctionByName(const SDK::UClass* ObjectClass, const char* ClassName, const char* FuncName)
{
    if (ObjectClass == nullptr || ClassName == nullptr || FuncName == nullptr)
    {
        return nullptr;
    }

    for (const SDK::UStruct* ClassNode = ObjectClass; ClassNode != nullptr; ClassNode = ClassNode->Super)
    {
        if (ClassNode->Name.ToString() != ClassName)
        {
            continue;
        }

        for (SDK::UField* Field = ClassNode->Children; Field != nullptr; Field = Field->Next)
        {
            if (Field->Class == nullptr)
            {
                continue;
            }

            if (Field->Name.ToString() == FuncName && Field->Class->Name.ToString() == "Function")
            {
                return static_cast<SDK::UFunction*>(Field);
            }
        }
    }

    return nullptr;
}

bool CallProcessEvent(SDK::UObject* Object, SDK::UFunction* Function, void* Params)
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
