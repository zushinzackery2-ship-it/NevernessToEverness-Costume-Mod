#include "Pch.h"
#include "CameraPatch.hpp"
#include "SdkHelpers.hpp"

namespace AntiFadeMod
{
namespace
{
struct FClassCache
{
    SDK::UClass* HTCharacterClass = nullptr;
    SDK::UClass* AbilityCharacterClass = nullptr;
    SDK::UClass* PlayerCharacterClass = nullptr;
};

constexpr float DisabledFadeDistance = 0.0f;
constexpr float DisabledHideDistance = 0.0f;

bool IsDisabledPlayerFadeInSettings(const SDK::FCameraSettings& Settings)
{
    return Settings.PlayerFadeDistance.bAmend
        && Settings.PlayerFadeDistance.TargetValue == DisabledFadeDistance
        && Settings.PlayerHideDistance.bAmend
        && Settings.PlayerHideDistance.TargetValue == DisabledHideDistance;
}

template <typename CameraSettingsType>
bool IsDisabledPlayerFadeInSettings(const CameraSettingsType& Settings)
{
    return IsDisabledPlayerFadeInSettings(static_cast<const SDK::FCameraSettings&>(Settings));
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

bool HasRequiredUnableFadeClasses(SDK::AHTPlayerCameraManager* CameraManager, const FClassCache& Classes)
{
    if (CameraManager == nullptr || CameraManager->ActorUnableFade.Max() <= 0)
    {
        return true;
    }

    return (Classes.HTCharacterClass == nullptr || ContainsUnableFadeClass(CameraManager->ActorUnableFade, Classes.HTCharacterClass))
        && (Classes.AbilityCharacterClass == nullptr || ContainsUnableFadeClass(CameraManager->ActorUnableFade, Classes.AbilityCharacterClass))
        && (Classes.PlayerCharacterClass == nullptr || ContainsUnableFadeClass(CameraManager->ActorUnableFade, Classes.PlayerCharacterClass));
}

bool AreCameraCollisionParamsDisabled(SDK::AHTPlayerCameraManager* CameraManager)
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
        if ((Type == SDK::ECameraCollisionType::Ect_HalfTrans || Type == SDK::ECameraCollisionType::Ect_FullTrans)
            && It->Value() != DisabledCollisionValue)
        {
            return false;
        }
    }

    return true;
}

bool AreCameraSettingsDisabled(SDK::AHTPlayerCameraManager* CameraManager)
{
    if (!IsDisabledPlayerFadeInSettings(CameraManager->RunSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->FightingSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->InLiftSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->JumpingSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->LockTargetSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->SpecialBoxSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->TimeClockSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->BossCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->TenacityBreakCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->SwimmingSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->ClimbingSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->ShopSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->CarTPSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->SitSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->BoardTrainSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->DialogueCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->LoadingCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->GlidingCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->LiningTargetCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->RollerCoasterCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->NormalCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->RideNormalCameraSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->LockNpcSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->SelfieSettings)
        || !IsDisabledPlayerFadeInSettings(CameraManager->CustomCameraSetting))
    {
        return false;
    }

    for (auto It = begin(CameraManager->InDoorCameraSettings); It != end(CameraManager->InDoorCameraSettings); ++It)
    {
        if (!IsDisabledPlayerFadeInSettings(It->Value()))
        {
            return false;
        }
    }

    for (auto It = begin(CameraManager->AimingCameraSettings); It != end(CameraManager->AimingCameraSettings); ++It)
    {
        if (!IsDisabledPlayerFadeInSettings(It->Value()))
        {
            return false;
        }
    }

    return true;
}

FClassCache BuildClassCache()
{
    FClassCache Classes{};
    Classes.HTCharacterClass = FindClassByName("HTCharacter");
    Classes.AbilityCharacterClass = FindClassByName("HTAbilityCharacter");
    Classes.PlayerCharacterClass = FindClassByName("HTPlayerCharacter");
    return Classes;
}
}

bool IsCameraManagerAntiFadeIntact(SDK::AHTPlayerCameraManager* CameraManager)
{
    if (CameraManager == nullptr)
    {
        return false;
    }

    const FClassCache Classes = BuildClassCache();
    return AreCameraSettingsDisabled(CameraManager)
        && CameraManager->PlayerPitchFadeCurve == nullptr
        && CameraManager->ActorFadeCurve == nullptr
        && CameraManager->CollisionFadeTotalDuration == 0.0f
        && CameraManager->PlayerFadeDistanceSquare == 0.0f
        && CameraManager->PlayerHideDistanceSquare == 0.0f
        && AreCameraCollisionParamsDisabled(CameraManager)
        && HasRequiredUnableFadeClasses(CameraManager, Classes);
}
}
