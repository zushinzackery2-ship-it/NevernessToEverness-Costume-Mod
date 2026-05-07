#include "Pch.h"
#include "CameraPatch.hpp"
#include "SdkHelpers.hpp"

namespace AntiFadeMod
{
namespace
{
struct FClassCache
{
    SDK::UClass* CameraManagerClass = nullptr;
    SDK::UClass* HTCharacterClass = nullptr;
    SDK::UClass* AbilityCharacterClass = nullptr;
    SDK::UClass* PlayerCharacterClass = nullptr;
};

constexpr float DisabledFadeDistance = 0.0f;
constexpr float DisabledHideDistance = 0.0f;

void DisablePlayerFadeInSettings(SDK::FCameraSettings& Settings, FPatchStats& Stats)
{
    ConfigureDisabledPlayerFade(Settings);
    ++Stats.CameraSettings;
}

template <typename CameraSettingsType>
void DisablePlayerFadeInSettings(CameraSettingsType& Settings, FPatchStats& Stats)
{
    DisablePlayerFadeInSettings(static_cast<SDK::FCameraSettings&>(Settings), Stats);
}

bool ContainsUnableFadeClass(const SDK::TArray<SDK::UClass*>& Classes, SDK::UClass* ClassToFind)
{
    if (ClassToFind == nullptr)
    {
        return false;
    }

    for (int Index = 0; Index < Classes.Num(); ++Index)
    {
        if (Classes.IsValidIndex(Index) && Classes[Index] == ClassToFind)
        {
            return true;
        }
    }

    return false;
}

void EnsureUnableFadeClass(SDK::TArray<SDK::UClass*>& Classes, SDK::UClass* ClassToAdd, FPatchStats& Stats)
{
    if (ClassToAdd == nullptr)
    {
        return;
    }

    if (ContainsUnableFadeClass(Classes, ClassToAdd))
    {
        ++Stats.UnableFadeClassesAlreadyPresent;
        return;
    }

    for (int Index = 0; Index < Classes.Num(); ++Index)
    {
        if (Classes.IsValidIndex(Index) && Classes[Index] == nullptr)
        {
            Classes[Index] = ClassToAdd;
            ++Stats.UnableFadeClassesAdded;
            return;
        }
    }

    if (Classes.Add(ClassToAdd))
    {
        ++Stats.UnableFadeClassesAdded;
    }
    else
    {
        ++Stats.UnableFadeAddNoSlack;
    }
}

void PatchUnableFadeClasses(SDK::AHTPlayerCameraManager* CameraManager, const FClassCache& Classes, FPatchStats& Stats)
{
    EnsureUnableFadeClass(CameraManager->ActorUnableFade, Classes.HTCharacterClass, Stats);
    EnsureUnableFadeClass(CameraManager->ActorUnableFade, Classes.AbilityCharacterClass, Stats);
    EnsureUnableFadeClass(CameraManager->ActorUnableFade, Classes.PlayerCharacterClass, Stats);
}

void PatchCameraCollisionParams(SDK::AHTPlayerCameraManager* CameraManager, FPatchStats& Stats)
{
    bool HasNoTransValue = false;
    float NoTransValue = 0.0f;

    for (auto It = begin(CameraManager->CameraCollisionParams); It != end(CameraManager->CameraCollisionParams); ++It)
    {
        if (It->Key() == SDK::ECameraCollisionType::Ect_NoTrans)
        {
            NoTransValue = It->Value();
            HasNoTransValue = true;
            break;
        }
    }

    const float DisabledCollisionValue = HasNoTransValue ? NoTransValue : 0.0f;

    for (auto It = begin(CameraManager->CameraCollisionParams); It != end(CameraManager->CameraCollisionParams); ++It)
    {
        const SDK::ECameraCollisionType Type = It->Key();
        if (Type != SDK::ECameraCollisionType::Ect_HalfTrans && Type != SDK::ECameraCollisionType::Ect_FullTrans)
        {
            continue;
        }

        if (It->Value() != DisabledCollisionValue)
        {
            ++Stats.CollisionParamEntriesChanged;
        }

        It->Value() = DisabledCollisionValue;
    }
}

void PatchCameraManagerSourceConfig(SDK::AHTPlayerCameraManager* CameraManager, const FClassCache& Classes, FPatchStats& Stats)
{
    if (CameraManager == nullptr)
    {
        return;
    }

    ++Stats.CameraManagers;

    DisablePlayerFadeInSettings(CameraManager->RunSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->FightingSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->InLiftSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->JumpingSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->LockTargetSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->SpecialBoxSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->TimeClockSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->BossCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->TenacityBreakCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->SwimmingSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->ClimbingSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->ShopSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->CarTPSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->SitSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->BoardTrainSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->DialogueCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->LoadingCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->GlidingCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->LiningTargetCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->RollerCoasterCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->NormalCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->RideNormalCameraSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->LockNpcSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->SelfieSettings, Stats);
    DisablePlayerFadeInSettings(CameraManager->CustomCameraSetting, Stats);

    for (auto It = begin(CameraManager->InDoorCameraSettings); It != end(CameraManager->InDoorCameraSettings); ++It)
    {
        DisablePlayerFadeInSettings(It->Value(), Stats);
        ++Stats.IndoorMapSettings;
    }

    for (auto It = begin(CameraManager->AimingCameraSettings); It != end(CameraManager->AimingCameraSettings); ++It)
    {
        DisablePlayerFadeInSettings(It->Value(), Stats);
        ++Stats.AimingMapSettings;
    }

    if (CameraManager->PlayerPitchFadeCurve != nullptr)
    {
        CameraManager->PlayerPitchFadeCurve = nullptr;
        ++Stats.PlayerPitchCurvesCleared;
    }

    if (CameraManager->ActorFadeCurve != nullptr)
    {
        CameraManager->ActorFadeCurve = nullptr;
        ++Stats.ActorFadeCurvesCleared;
    }

    if (CameraManager->CollisionFadeTotalDuration != 0.0f)
    {
        CameraManager->CollisionFadeTotalDuration = 0.0f;
        ++Stats.CollisionDurationsCleared;
    }

    if (CameraManager->PlayerFadeDistanceSquare != 0.0f)
    {
        CameraManager->PlayerFadeDistanceSquare = 0.0f;
        ++Stats.CachedDistanceValuesCleared;
    }

    if (CameraManager->PlayerHideDistanceSquare != 0.0f)
    {
        CameraManager->PlayerHideDistanceSquare = 0.0f;
        ++Stats.CachedDistanceValuesCleared;
    }

    PatchCameraCollisionParams(CameraManager, Stats);
    PatchUnableFadeClasses(CameraManager, Classes, Stats);
}

FClassCache BuildClassCache()
{
    FClassCache Classes{};
    Classes.CameraManagerClass = FindClassByName("HTPlayerCameraManager");
    Classes.HTCharacterClass = FindClassByName("HTCharacter");
    Classes.AbilityCharacterClass = FindClassByName("HTAbilityCharacter");
    Classes.PlayerCharacterClass = FindClassByName("HTPlayerCharacter");
    return Classes;
}
}

void ConfigureDisabledPlayerFade(SDK::FCameraSettings& Settings)
{
    Settings.PlayerFadeDistance.bAmend = true;
    Settings.PlayerFadeDistance.TargetValue = DisabledFadeDistance;
    Settings.PlayerHideDistance.bAmend = true;
    Settings.PlayerHideDistance.TargetValue = DisabledHideDistance;
}

FPatchStats PatchCurrentCameraManager(SDK::AHTPlayerCameraManager* CurrentCameraManager)
{
    FPatchStats Stats{};
    const FClassCache Classes = BuildClassCache();
    PatchCameraManagerSourceConfig(CurrentCameraManager, Classes, Stats);

    return Stats;
}
}

