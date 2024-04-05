#pragma once
#include <Igniter.h>
#include <Core/Result.h>
#include <Asset/Material.h>

namespace ig::details
{
    enum class EMakeDefaultMatStatus
    {
        Success,
        InvalidAssetInfo
    };
}

namespace ig
{
    enum class EMaterialLoadStatus
    {
        Success,
        InvalidAssetInfo,
        AssetTypeMismatch,
        FailedLoadDiffuse,         /* #sy_todo 추후 엔진 기본 텍스처가 추가되면 대체 */
    };

    class AssetManager;
    class MaterialLoader final
    {
        friend class AssetManager;

    public:
        MaterialLoader(AssetManager& assetManager);
        MaterialLoader(const MaterialLoader&) = delete;
        MaterialLoader(MaterialLoader&&) noexcept = delete;
        ~MaterialLoader() = default;

        MaterialLoader& operator=(const MaterialLoader&) = delete;
        MaterialLoader& operator=(MaterialLoader&&) noexcept = delete;

    private:
        Result<Material, EMaterialLoadStatus> Load(const Material::Desc& desc);
        Result<Material, details::EMakeDefaultMatStatus> MakeDefault(const AssetInfo& assetInfo);

    private:
        AssetManager& assetManager;
    };
} // namespace ig