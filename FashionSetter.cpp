#include "Pch.h"

#include "FashionSetter.h"

#include "AppearanceData.h"
#include "Log.h"
#include "SdkRuntimeHelpers.h"

namespace CurrentFashionSetter
{
    extern std::unordered_map<std::string, SDK::FName> g_LastFashionByCharacter;

    struct OnRepFashionParams
    {
        SDK::FName OldFashionID;
    };

    struct ResetCharacterMeshParams
    {
        bool ResetToCapsuleCenter;
    };

    struct NameTagParams
    {
        SDK::FName NameTag;
    };

    struct LoadAssetBlockingParams
    {
        SDK::TSoftObjectPtr<SDK::UObject> Asset;
        SDK::UObject* ReturnValue;
    };

    struct ConvStringToNameParams
    {
        SDK::FString InString;
        SDK::FName ReturnValue;
    };

    std::string NameListToString(const SDK::TArray<SDK::FName>& names)
    {
        std::string result;
        for (const SDK::FName& name : names)
        {
            if (!result.empty())
            {
                result += ",";
            }
            result += name.ToString();
        }
        return result;
    }

    void LogTargetAppearanceDetails(SDK::UHTPlayerAppearance* appearance, std::ofstream& log)
    {
        if (!IsPlayerAppearanceObject(appearance))
        {
            LogLine(log, "target appearance invalid for mesh detail object=" + ObjectName(appearance));
            return;
        }

        LogLine(log, "target appearance object=" + ObjectName(appearance));
        LogLine(log, "target FashionMeshData mesh=" + ObjectName(appearance->FashionMeshData.CharacterMesh)
            + " anim=" + ObjectName(appearance->FashionMeshData.AnimInstance));
        LogLine(log, "target ArrayFashionAttachedMeshData count="
            + std::to_string(appearance->ArrayFashionAttachedMeshData.Num()));
        for (const SDK::FAttachedMeshData& attached : appearance->ArrayFashionAttachedMeshData)
        {
            LogLine(log, "  fashion attached socket=" + attached.SocketName.ToString()
                + " mesh=" + ObjectName(attached.CharacterMesh)
                + " tags=[" + NameListToString(attached.MeshComponentOwnedTags) + "]");
        }

        LogLine(log, "target ArrayDynamicAttachedMeshData count="
            + std::to_string(appearance->ArrayDynamicAttachedMeshData.Num()));
        for (const SDK::FExtraAttachedMeshData& extra : appearance->ArrayDynamicAttachedMeshData)
        {
            LogLine(log, "  dynamic attached tag=" + extra.TagName.ToString()
                + " entries=" + std::to_string(extra.ArrayExtraAttachedMeshData.Num()));
        }
    }

    SDK::UHTPlayerAppearance* LoadTargetAppearance(const FashionCandidate& target, std::ofstream& log)
    {
        if (target.IsDefault)
        {
            LogLine(log, "target default PlayerAppearance path="
                + SoftObjectPathToString(target.DefaultPlayerAppearanceAsset));

            SDK::UObject* beforeObject = target.DefaultPlayerAppearanceAsset.Get();
            LogLine(log, "target default PlayerAppearance before load=" + ObjectName(beforeObject));
            if (SDK::UHTPlayerAppearance* before = AsPlayerAppearance(beforeObject, log))
            {
                return before;
            }

            SDK::UFunction* loadAsset = SDK::UKismetSystemLibrary::StaticClass()->GetFunction("KismetSystemLibrary", "LoadAsset_Blocking");
            LoadAssetBlockingParams params = {};
            params.Asset = CopySoftObjectPtr(target.DefaultPlayerAppearanceAsset);
            if (!CallProcessEvent(SDK::UKismetSystemLibrary::GetDefaultObj(), loadAsset, &params, log))
            {
                LogLine(log, "LoadAsset_Blocking call failed for default appearance");
                return nullptr;
            }

            SDK::UObject* loadedObject = params.ReturnValue;
            LogLine(log, "target default PlayerAppearance after load=" + ObjectName(loadedObject));
            return AsPlayerAppearance(loadedObject, log);
        }

        if (target.FashionData == nullptr)
        {
            LogLine(log, "target FFashionData missing; skip LoadAsset_Blocking");
            return nullptr;
        }

        LogLine(log, "target PlayerAppearance path="
            + SoftObjectPathToString(target.FashionData->PlayerAppearanceAsset));

        SDK::UObject* beforeObject = target.FashionData->PlayerAppearanceAsset.Get();
        LogLine(log, "target PlayerAppearance before load=" + ObjectName(beforeObject));
        if (SDK::UHTPlayerAppearance* before = AsPlayerAppearance(beforeObject, log))
        {
            return before;
        }

        SDK::UFunction* loadAsset = SDK::UKismetSystemLibrary::StaticClass()->GetFunction("KismetSystemLibrary", "LoadAsset_Blocking");
        LoadAssetBlockingParams params = {};
        params.Asset = CopySoftObjectPtr(target.FashionData->PlayerAppearanceAsset);
        if (!CallProcessEvent(SDK::UKismetSystemLibrary::GetDefaultObj(), loadAsset, &params, log))
        {
            LogLine(log, "LoadAsset_Blocking call failed");
            return nullptr;
        }

        SDK::UObject* loadedObject = params.ReturnValue;
        LogLine(log, "target PlayerAppearance after load=" + ObjectName(loadedObject));
        return AsPlayerAppearance(loadedObject, log);
    }

    bool TryConvertStringToName(const wchar_t* text, SDK::FName& result, std::ofstream& log)
    {
        SDK::UFunction* function = SDK::UKismetStringLibrary::StaticClass()->GetFunction("KismetStringLibrary", "Conv_StringToName");
        ConvStringToNameParams params = {};
        params.InString = SDK::FString(text);
        if (!CallProcessEvent(SDK::UKismetStringLibrary::GetDefaultObj(), function, &params, log))
        {
            LogLine(log, "Conv_StringToName call failed");
            return false;
        }

        if (params.ReturnValue.IsNone())
        {
            LogLine(log, "Conv_StringToName returned None");
            return false;
        }

        result = params.ReturnValue;
        return true;
    }

    void LogCharacterAppearanceState(SDK::AHTPlayerCharacter* character, const char* label, std::ofstream& log)
    {
        if (character == nullptr)
        {
            return;
        }

        LogLine(log, std::string(label)
            + " FashionID=" + character->FashionID.ToString()
            + " CurPlayerAppearance=" + ObjectName(character->CurPlayerAppearance)
            + " SoftCurPlayerAppearance.Get=" + ObjectName(character->SoftCurPlayerAppearance.Get()));
    }

    bool CallResetCharacterMesh(SDK::AHTPlayerCharacter* character, bool resetToCapsuleCenter, std::ofstream& log)
    {
        SDK::UFunction* resetMesh = character->Class->GetFunction("HTPlayerCharacter", "ResetCharacterMesh");
        ResetCharacterMeshParams params = {};
        params.ResetToCapsuleCenter = resetToCapsuleCenter;
        return CallProcessEvent(character, resetMesh, &params, log);
    }

    bool CallNameTagFunction(SDK::AHTPlayerCharacter* character, const char* functionName, const SDK::FName& tag, std::ofstream& log)
    {
        SDK::UFunction* function = character->Class->GetFunction("HTPlayerCharacter", functionName);
        NameTagParams params = {};
        params.NameTag = tag;
        return CallProcessEvent(character, function, &params, log);
    }

    template<typename TargetSoftType, typename SourceSoftType>
    void CopySoftObjectPtrInto(TargetSoftType& target, const SourceSoftType& source)
    {
        static_assert(sizeof(target) == sizeof(source));
        memcpy(&target, &source, sizeof(target));
    }

    void SyncCharacterAppearanceFields(SDK::AHTPlayerCharacter* character,
        const FashionCandidate& target,
        SDK::UHTPlayerAppearance* targetAppearance,
        std::ofstream& log)
    {
        if (character == nullptr || targetAppearance == nullptr)
        {
            LogLine(log, "skip direct appearance field sync: invalid character, fashion data, or appearance");
            return;
        }

        character->FashionID = target.RowName;
        character->CurPlayerAppearance = targetAppearance;
        if (target.IsDefault)
        {
            CopySoftObjectPtrInto(character->SoftCurPlayerAppearance, target.DefaultPlayerAppearanceAsset);
        }
        else if (target.FashionData != nullptr)
        {
            CopySoftObjectPtrInto(character->SoftCurPlayerAppearance, target.FashionData->PlayerAppearanceAsset);
        }
        else
        {
            LogLine(log, "skip direct appearance field sync: missing non-default fashion data");
            return;
        }
        memcpy(&character->CurMeshData, &targetAppearance->FashionMeshData, sizeof(character->CurMeshData));

        character->ChangeAvatarData.CharacterID = character->DefaultCharacterID;
        character->ChangeAvatarData.PlayerAppearance = targetAppearance;
        if (target.IsDefault)
        {
            CopySoftObjectPtrInto(character->ChangeAvatarData.SoftPlayerAppearance, target.DefaultPlayerAppearanceAsset);
        }
        else if (target.FashionData != nullptr)
        {
            CopySoftObjectPtrInto(character->ChangeAvatarData.SoftPlayerAppearance, target.FashionData->PlayerAppearanceAsset);
        }

        LogLine(log, "direct appearance field sync applied target=" + target.RowName.ToString()
            + " mesh=" + ObjectName(targetAppearance->FashionMeshData.CharacterMesh)
            + " anim=" + ObjectName(targetAppearance->FashionMeshData.AnimInstance)
            + " ChangeAvatarData.CharacterID=" + character->ChangeAvatarData.CharacterID.ToString()
            + " ChangeAvatarData.PlayerAppearance=" + ObjectName(character->ChangeAvatarData.PlayerAppearance));
    }

    void RefreshDynamicAttachedMeshTags(SDK::AHTPlayerCharacter* character, SDK::UHTPlayerAppearance* appearance, std::ofstream& log)
    {
        if (!IsValidObject(appearance))
        {
            LogLine(log, "skip attached mesh refresh: target appearance not loaded");
            return;
        }

        if (appearance->ArrayDynamicAttachedMeshData.Num() == 0)
        {
            LogLine(log, "skip attached mesh refresh: no dynamic attached mesh tags");
            return;
        }

        for (const SDK::FExtraAttachedMeshData& extra : appearance->ArrayDynamicAttachedMeshData)
        {
            LogLine(log, "refresh attached mesh tag=" + extra.TagName.ToString());
            CallNameTagFunction(character, "ClearCharacterAttachedMeshByTag", extra.TagName, log);
            CallNameTagFunction(character, "SetCharacterAttachedMeshByTag", extra.TagName, log);
        }
    }

    void RefreshPreviewWidgetCharacter(SDK::AHTPlayerCharacter* character, const SDK::FName& fashionId, std::ofstream& log)
    {
        const int count = SDK::UObject::GObjects->Num();

        for (int index = 0; index < count; ++index)
        {
            SDK::UObject* object = SDK::UObject::GObjects->GetByIndex(index);
            if (!IsValidObject(object) || object->Class->GetName() != "HTPreviewWidgetCharacter")
            {
                continue;
            }

            auto* previewWidget = reinterpret_cast<SDK::UHTPreviewWidgetCharacter*>(object);
            SDK::UFunction* initPreview = previewWidget->Class->GetFunction("HTPreviewWidgetCharacter", "InitSoloPreview");
            if (initPreview == nullptr)
            {
                continue;
            }

            SDK::Params::HTPreviewWidgetCharacter_InitSoloPreview params = {};
            params.InCharacterInfo.ItemID = character->DefaultCharacterID;
            params.InCharacterInfo.ItemNetID = character->UniqueID;
            params.InCharacterInfo.FashionID = fashionId;

            LogLine(log, "RefreshPreviewWidgetCharacter widget=" + PointerString(object)
                + " CharacterID=" + character->DefaultCharacterID.ToString()
                + " FashionID=" + fashionId.ToString());
            CallProcessEvent(previewWidget, initPreview, &params, log);
        }
    }

    void ApplyFashionCandidateToCharacter(SDK::AHTPlayerCharacter* character, const SDK::FName& oldFashion, const FashionCandidate& target, std::ofstream& log)
    {
        SDK::UHTPlayerAppearance* targetAppearance = LoadTargetAppearance(target, log);
        LogTargetAppearanceDetails(targetAppearance, log);
        if (!IsPlayerAppearanceObject(targetAppearance))
        {
            LogLine(log, "target appearance is unavailable; skip fashion apply to avoid host fatal exit");
            return;
        }

        SyncCharacterAppearanceFields(character, target, targetAppearance, log);
        LogCharacterAppearanceState(character, "after direct field sync", log);

        SDK::UFunction* onRepFashion = character->Class->GetFunction("HTPlayerCharacter", "OnRep_FashionID");
        OnRepFashionParams onRepParams = {};
        onRepParams.OldFashionID = oldFashion;
        CallProcessEvent(character, onRepFashion, &onRepParams, log);
        LogCharacterAppearanceState(character, "after OnRep_FashionID", log);

        CallResetCharacterMesh(character, false, log);
        LogCharacterAppearanceState(character, "after ResetCharacterMesh(false)", log);
        RefreshDynamicAttachedMeshTags(character, targetAppearance, log);
        LogCharacterAppearanceState(character, "after attached mesh tag refresh", log);

        RefreshPreviewWidgetCharacter(character, target.RowName, log);

        g_LastFashionByCharacter[character->DefaultCharacterID.ToString()] = target.RowName;

        LogLine(log, "skip BPOnBuildCharacterMeshFinish: direct manual call raised protected SEH in previous run");
    }

    void RunFashionSwapOnGameThread(int direction)
    {
        std::ofstream log = OpenAppendLog();
        LogLine(log, "RunFashionSwapOnGameThread start direction=" + std::to_string(direction));

        SDK::FName::InitInternal();
        SDK::UObject::GObjects.GetTypedPtr();

        SDK::AHTPlayerCharacter* character = GetCurrentPlayerCharacter(log);
        if (character == nullptr)
        {
            return;
        }

        const SDK::FName oldFashion = character->FashionID;
        const SDK::FName characterId = character->DefaultCharacterID;
        LogLine(log, "CharacterID=" + characterId.ToString());
        LogLine(log, "OldFashionID=" + oldFashion.ToString());
        LogCharacterAppearanceState(character, "before", log);

        FashionCandidate defaultCandidate = {};
        SDK::FName defaultFashionId = {};
        if (!TryConvertStringToName(L"DefaultFashion", defaultFashionId, log))
        {
            return;
        }
        defaultCandidate.Name = "DefaultFashion";
        defaultCandidate.RowName = defaultFashionId;
        defaultCandidate.IsDefault = true;
        defaultCandidate.DefaultPlayerAppearanceAsset = character->DefaultPlayerAppearanceAsset;
        LogLine(log, "default candidate FashionID=" + defaultCandidate.RowName.ToString()
            + " PlayerAppearance path=" + SoftObjectPathToString(defaultCandidate.DefaultPlayerAppearanceAsset)
            + " loaded=" + ObjectName(defaultCandidate.DefaultPlayerAppearanceAsset.Get()));

        SDK::UDataTable* appearanceTable = FindAppearanceDataTable(characterId, log);
        if (appearanceTable == nullptr)
        {
            return;
        }

        FashionCandidate target = {};
        if (!SelectFashionByDirection(appearanceTable, characterId, oldFashion, &defaultCandidate, direction, target, log))
        {
            return;
        }

        ApplyFashionCandidateToCharacter(character, oldFashion, target, log);
        LogLine(log, "done");
    }

    void ApplyFashionById(const SDK::FName& targetFashionId)
    {
        std::ofstream log = OpenAppendLog();
        LogLine(log, "ApplyFashionById target=" + targetFashionId.ToString());

        SDK::FName::InitInternal();
        SDK::UObject::GObjects.GetTypedPtr();

        SDK::AHTPlayerCharacter* character = GetCurrentPlayerCharacter(log);
        if (character == nullptr)
        {
            return;
        }

        const SDK::FName oldFashion = character->FashionID;
        const SDK::FName characterId = character->DefaultCharacterID;
        LogLine(log, "CharacterID=" + characterId.ToString());
        LogLine(log, "OldFashionID=" + oldFashion.ToString());
        LogCharacterAppearanceState(character, "before", log);

        FashionCandidate defaultCandidate = {};
        SDK::FName defaultFashionId = {};
        if (!TryConvertStringToName(L"DefaultFashion", defaultFashionId, log))
        {
            return;
        }
        defaultCandidate.Name = "DefaultFashion";
        defaultCandidate.RowName = defaultFashionId;
        defaultCandidate.IsDefault = true;
        defaultCandidate.DefaultPlayerAppearanceAsset = character->DefaultPlayerAppearanceAsset;
        LogLine(log, "default candidate FashionID=" + defaultCandidate.RowName.ToString()
            + " PlayerAppearance path=" + SoftObjectPathToString(defaultCandidate.DefaultPlayerAppearanceAsset)
            + " loaded=" + ObjectName(defaultCandidate.DefaultPlayerAppearanceAsset.Get()));

        SDK::UDataTable* appearanceTable = FindAppearanceDataTable(characterId, log);
        if (appearanceTable == nullptr)
        {
            return;
        }

        FashionCandidate target = {};
        if (!FindFashionCandidateById(appearanceTable, characterId, targetFashionId, &defaultCandidate, target, log))
        {
            g_LastFashionByCharacter.erase(characterId.ToString());
            return;
        }

        ApplyFashionCandidateToCharacter(character, oldFashion, target, log);
        LogLine(log, "done");
    }
}
