#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/Core/Result.h"
#include "Igniter/Core/String.h"
#include "Igniter/Asset/Texture.h"

namespace ig
{
    struct MaterialAssetCreateDesc final
    {
      public:
        String DiffuseVirtualPath{};
    };

    struct MaterialAssetLoadDesc final
    {
      public:
        Json& Serialize(Json& archive) const;
        const Json& Deserialize(const Json& archive);

      public:
        Guid DiffuseTexGuid{DefaultTextureGuid};
    };

    class AssetManager;

    class Material final
    {
      public:
        using ImportDesc = MaterialAssetCreateDesc;
        using LoadDesc = MaterialAssetLoadDesc;
        using Desc = AssetDesc<Material>;

        friend class AssetManager;

      public:
        Material(AssetManager& assetManager, const Desc& snapshot, const Handle<Texture> diffuse);
        Material(const Material&) = delete;
        Material(Material&&) noexcept = default;
        ~Material();

        Material& operator=(const Material&) = delete;
        Material& operator=(Material&& rhs) noexcept;

        [[nodiscard]] const Desc& GetSnapshot() const { return snapshot; }
        [[nodiscard]] Handle<Texture> GetDiffuse() const { return diffuse; }

      private:
        void Destroy();

      public:
        /* #sy_wip Common 헤더로 이동 */
        static constexpr std::string_view EngineDefault{"Engine\\Default"};

      private:
        AssetManager* assetManager{nullptr};
        Desc snapshot{};
        Handle<Texture> diffuse{};
    };
} // namespace ig
