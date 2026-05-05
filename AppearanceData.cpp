#include "Pch.h"

#include "AppearanceData.h"

#include "Log.h"

namespace CurrentFashionSetter
{
    struct InstancedStructRaw
    {
        const SDK::UObject* ScriptStruct;
        const void* Data;
    };

    bool IsValidObject(const SDK::UObject* object)
    {
        return object != nullptr && object->Class != nullptr;
    }

    std::string ObjectName(const SDK::UObject* object)
    {
        if (!IsValidObject(object))
        {
            return PointerString(object);
        }

        return object->GetFullName();
    }

    SDK::AHTPlayerCharacter* GetCurrentPlayerCharacter(std::ofstream& log)
    {
        auto* world = reinterpret_cast<SDK::UWorld*>(SDK::InSDKUtils::GetGWorld());
        LogLine(log, "GWorld=" + PointerString(world));
        if (!IsValidObject(world) || world->OwningGameInstance == nullptr)
        {
            LogLine(log, "invalid world or OwningGameInstance");
            return nullptr;
        }

        SDK::UGameInstance* gameInstance = world->OwningGameInstance;
        if (!gameInstance->LocalPlayers.IsValidIndex(0))
        {
            LogLine(log, "LocalPlayers[0] is invalid");
            return nullptr;
        }

        SDK::ULocalPlayer* localPlayer = gameInstance->LocalPlayers[0];
        LogLine(log, "LocalPlayer=" + PointerString(localPlayer));
        SDK::UPlayer* localPlayerBase = IsValidObject(localPlayer) ? localPlayer : nullptr;
        if (localPlayerBase == nullptr || localPlayerBase->PlayerController == nullptr)
        {
            LogLine(log, "invalid local player or PlayerController");
            return nullptr;
        }

        SDK::APlayerController* controller = localPlayerBase->PlayerController;
        SDK::APawn* pawn = controller->AcknowledgedPawn;
        LogLine(log, "AcknowledgedPawn=" + PointerString(pawn));
        if (!IsValidObject(pawn))
        {
            LogLine(log, "invalid acknowledged pawn");
            return nullptr;
        }

        auto* character = reinterpret_cast<SDK::AHTPlayerCharacter*>(pawn);
        if (character->Class->GetFunction("HTPlayerCharacter", "SetFashionID") == nullptr)
        {
            LogLine(log, "pawn class chain has no HTPlayerCharacter::SetFashionID");
            return nullptr;
        }

        LogLine(log, "PawnClass=" + character->Class->GetName());
        return character;
    }

    InstancedStructRaw ReadInstancedStruct(const SDK::FInstancedStruct& value)
    {
        return *reinterpret_cast<const InstancedStructRaw*>(&value);
    }

    const SDK::FFashionData* GetFashionData(const SDK::FStaticAppearanceData* data, const SDK::UObject** scriptStruct)
    {
        if (data == nullptr)
        {
            return nullptr;
        }

        const InstancedStructRaw raw = ReadInstancedStruct(data->AppearanceData);
        if (scriptStruct != nullptr)
        {
            *scriptStruct = raw.ScriptStruct;
        }

        if (raw.Data == nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<const SDK::FFashionData*>(raw.Data);
    }

    int CountMatchingFashions(SDK::UDataTable* table, const SDK::FName& characterId)
    {
        if (!IsValidObject(table) || !table->RowMap.IsValid())
        {
            return 0;
        }

        int matches = 0;
        for (const auto& row : table->RowMap)
        {
            const auto* data = reinterpret_cast<const SDK::FStaticAppearanceData*>(row.Value());
            if (data != nullptr
                && data->AppearanceType == SDK::EAppearanceType::Fashion
                && data->CharacterID == characterId)
            {
                ++matches;
            }
        }
        return matches;
    }

    SDK::UDataTable* FindAppearanceDataTable(const SDK::FName& characterId, std::ofstream& log)
    {
        SDK::UObject::GObjects.GetTypedPtr();
        const int count = SDK::UObject::GObjects->Num();
        LogLine(log, "GObjects.Num=" + std::to_string(count));

        for (int index = 0; index < count; ++index)
        {
            SDK::UObject* object = SDK::UObject::GObjects->GetByIndex(index);
            if (!IsValidObject(object))
            {
                continue;
            }

            if (object->Class->GetName() == "CharacterDataAsset")
            {
                auto* asset = reinterpret_cast<SDK::UCharacterDataAsset*>(object);
                const int matches = CountMatchingFashions(asset->AppearanceDataTable, characterId);
                LogLine(log, "CharacterDataAsset=" + PointerString(object) + " name=" + object->GetName()
                    + " cdo=" + std::to_string(object->IsDefaultObject()) + " table="
                    + PointerString(asset->AppearanceDataTable) + " matches=" + std::to_string(matches));
                if (!object->IsDefaultObject() && matches > 0)
                {
                    return asset->AppearanceDataTable;
                }
            }
        }

        for (int index = 0; index < count; ++index)
        {
            SDK::UObject* object = SDK::UObject::GObjects->GetByIndex(index);
            if (!IsValidObject(object) || object->IsDefaultObject() || object->Class->GetName() != "DataTable")
            {
                continue;
            }

            auto* table = reinterpret_cast<SDK::UDataTable*>(object);
            const int matches = CountMatchingFashions(table, characterId);
            if (matches > 0)
            {
                LogLine(log, "fallback DataTable=" + PointerString(object) + " name=" + object->GetName()
                    + " matches=" + std::to_string(matches));
                return table;
            }
        }

        LogLine(log, "valid AppearanceDataTable not found");
        return nullptr;
    }

    bool SelectFashionByDirection(SDK::UDataTable* table, const SDK::FName& characterId,
        const SDK::FName& currentFashionId, const FashionCandidate* defaultCandidate,
        int direction, FashionCandidate& target, std::ofstream& log)
    {
        if (!IsValidObject(table))
        {
            LogLine(log, "AppearanceDataTable is invalid");
            return false;
        }

        std::vector<FashionCandidate> candidates;
        for (const auto& row : table->RowMap)
        {
            const SDK::FName& rowName = row.Key();
            const auto* data = reinterpret_cast<const SDK::FStaticAppearanceData*>(row.Value());
            if (data == nullptr)
            {
                continue;
            }

            if (data->AppearanceType != SDK::EAppearanceType::Fashion || data->CharacterID != characterId)
            {
                continue;
            }

            const SDK::UObject* scriptStruct = nullptr;
            const SDK::FFashionData* fashionData = GetFashionData(data, &scriptStruct);
            if (fashionData == nullptr)
            {
                LogLine(log, "skip fashion row without FFashionData row=" + rowName.ToString()
                    + " script=" + ObjectName(scriptStruct));
                continue;
            }
            candidates.push_back({ rowName.ToString(), rowName, data, fashionData, scriptStruct });
        }

        if (defaultCandidate != nullptr)
        {
            candidates.push_back(*defaultCandidate);
        }

        std::sort(candidates.begin(), candidates.end(), [](const FashionCandidate& left, const FashionCandidate& right)
        {
            return left.Name < right.Name;
        });

        LogLine(log, "matching fashion count=" + std::to_string(candidates.size()));
        for (size_t index = 0; index < candidates.size(); ++index)
        {
            const FashionCandidate& candidate = candidates[index];
            const void* loaded = candidate.FashionData != nullptr
                ? candidate.FashionData->PlayerAppearanceAsset.Get()
                : nullptr;
            LogLine(log, std::to_string(index + 1) + ". " + candidate.Name
                + " script=" + ObjectName(candidate.ScriptStruct)
                + " data=" + PointerString(candidate.FashionData)
                + " loadedPlayerAppearance=" + PointerString(loaded));
        }

        if (candidates.empty())
        {
            LogLine(log, "no fashion rows for current character");
            return false;
        }

        size_t currentIndex = candidates.size();
        for (size_t index = 0; index < candidates.size(); ++index)
        {
            if (candidates[index].RowName == currentFashionId)
            {
                currentIndex = index;
                break;
            }
        }

        size_t targetIndex = 0;
        if (currentIndex == candidates.size())
        {
            targetIndex = direction >= 0 ? 0 : candidates.size() - 1;
            LogLine(log, "current FashionID not found in candidates; selecting boundary target");
        }
        else if (direction >= 0)
        {
            targetIndex = (currentIndex + 1) % candidates.size();
        }
        else
        {
            targetIndex = (currentIndex + candidates.size() - 1) % candidates.size();
        }

        target = candidates[targetIndex];
        LogLine(log, "selected fashion direction=" + std::to_string(direction)
            + " current=" + currentFashionId.ToString()
            + " target=" + target.RowName.ToString()
            + " index=" + std::to_string(targetIndex + 1)
            + "/" + std::to_string(candidates.size()));
        return true;
    }

    bool SelectFashionByDirection(SDK::UDataTable* table, const SDK::FName& characterId,
        const SDK::FName& currentFashionId, int direction, FashionCandidate& target, std::ofstream& log)
    {
        return SelectFashionByDirection(table, characterId, currentFashionId, nullptr, direction, target, log);
    }
}
