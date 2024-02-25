#pragma once
#include <Core/Handle.h>
#include <Core/Container.h>
#include <Core/String.h>

namespace fe
{
	enum class EAssetType
	{
		Texture,
		StaticMesh,
		Model,
		Shader,
		Audio,
		Script,
		// Scene
	};

	class Texture;
	struct TextureImportConfig;
	class StaticMesh;
	struct StaticMeshImportConfig;
	class Model;
	struct ModelImportConfig;
	class Shader;
	struct ShaderImportConfig;
	class Audio;
	struct AudioImportConfig;
	class Script;
	struct ScriptImportConfig;
	class AssetManager final
	{
		template <typename AssetType>
		using AssetRef = WeakHandle<AssetType, AssetManager*>;

	public:
		AssetManager();
		AssetManager(const AssetManager&) = delete;
		AssetManager(AssetManager&&) noexcept = delete;
		~AssetManager();

		AssetManager& operator=(const AssetManager&) = delete;
		AssetManager& operator=(AssetManager&&) noexcept = delete;

		bool ImportTexture(const String path, const TextureImportConfig& config);
		AssetRef<Texture> LoadTexture(const xg::Guid& guid);
		AssetRef<Texture> LoadTexture(const String path);
		void			  UnloadTexture(const xg::Guid& guid);

	private:
		robin_hood::unordered_map<String, xg::Guid> pathGuidTable;
		robin_hood::unordered_map<xg::Guid, UniqueHandle<Texture, AssetManager*>> guidTextureTable;

	};

} // namespace fe