#pragma once
#include <Core/Container.h>
#include <Core/String.h>
#include <Core/Assert.h>

namespace ig
{
    template <uint64_t DerivedVersion>
    struct Serializable
    {
    public:
        [[nodiscard]]
        bool IsLatestVersion() const
        {
            return Version == LatestVersion;
        }

        virtual json& Serialize(json& archive) const
        {
            const Serializable& serializable = *this;
            FE_SERIALIZE_JSON(Serializable, archive, serializable, LatestVersion);
            return archive;
        }

        virtual const json& Deserialize(const json& archive)
        {
            Serializable& serializable = *this;
            FE_DESERIALIZE_JSON(Serializable, archive, serializable, Version);
            check(serializable.IsLatestVersion());
            return archive;
        }

    private:
        constexpr static uint64_t BaseVersion = 0;

    public:
        constexpr static uint64_t LatestVersion = BaseVersion + DerivedVersion;
        uint64_t Version = 0;
    };

    template <typename T>
    [[nodiscard]] T MakeVersionedDefault()
    {
        T newData{};
        newData.Version = T::LatestVersion;
        return newData;
    }
} // namespace ig