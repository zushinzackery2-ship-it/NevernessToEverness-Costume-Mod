#include "Pch.h"

namespace SDK
{
    uintptr_t InSDKUtils::GetImageBase()
    {
        return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    }

    UClass* BasicFilesImpleUtils::FindClassByName(const std::string& name, bool bByFullName)
    {
        return bByFullName ? UObject::FindClass(name) : UObject::FindClassFast(name);
    }

    UClass* BasicFilesImpleUtils::FindClassByFullName(const std::string& name)
    {
        return UObject::FindClass(name);
    }

    std::string BasicFilesImpleUtils::GetObjectName(UClass* klass)
    {
        return klass != nullptr ? klass->GetName() : std::string();
    }

    int32 BasicFilesImpleUtils::GetObjectIndex(UClass* klass)
    {
        return klass != nullptr ? klass->Index : -1;
    }

    uint64 BasicFilesImpleUtils::GetObjFNameAsUInt64(UClass* klass)
    {
        return klass != nullptr ? *reinterpret_cast<uint64*>(&klass->Name) : 0;
    }

    UObject* BasicFilesImpleUtils::GetObjectByIndex(int32 index)
    {
        return UObject::GObjects->GetByIndex(index);
    }

    UFunction* BasicFilesImpleUtils::FindFunctionByFName(const FName* name)
    {
        if (name == nullptr)
        {
            return nullptr;
        }

        for (int index = 0; index < UObject::GObjects->Num(); ++index)
        {
            UObject* object = UObject::GObjects->GetByIndex(index);
            if (object != nullptr && object->Name == *name)
            {
                return static_cast<UFunction*>(object);
            }
        }

        return nullptr;
    }

    UObject* FWeakObjectPtr::Get() const
    {
        return UObject::GObjects->GetByIndex(ObjectIndex);
    }

    UObject* FWeakObjectPtr::operator->() const
    {
        return Get();
    }

    bool FWeakObjectPtr::operator==(const FWeakObjectPtr& other) const
    {
        return ObjectIndex == other.ObjectIndex;
    }

    bool FWeakObjectPtr::operator!=(const FWeakObjectPtr& other) const
    {
        return ObjectIndex != other.ObjectIndex;
    }

    bool FWeakObjectPtr::operator==(const UObject* other) const
    {
        return other != nullptr && ObjectIndex == other->Index;
    }

    bool FWeakObjectPtr::operator!=(const UObject* other) const
    {
        return other == nullptr || ObjectIndex != other->Index;
    }
}
