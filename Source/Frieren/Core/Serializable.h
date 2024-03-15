#pragma once
#include <Core/Container.h>
#include <Core/String.h>
#include <Core/Assert.h>

namespace fe
{
    template <uint64_t CurrentVersion>
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
            const Serializable& data = *this;
            check(metadata.IsLatestVersion());
            FE_SERIALIZE_JSON(Serializable, archive, data, Version);
            return archive;
        }

        virtual const json& Deserialize(const json& archive)
        {
            ResourceMetadata& metadata = *this;
            FE_DESERIALIZE_JSON(Serializable, archive, metadata, Version);
            check(metadata.IsLatestVersion());
            return archive;
        }

    private:
        constexpr static uint64_t BaseVersion = 1;
        constexpr static uint64_t LatestVersion = BaseVersion + CurrentVersion;

        uint64_t Version = 0;
    };
} // namespace fe