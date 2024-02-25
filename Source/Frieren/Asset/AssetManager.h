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
		AssetManager() = default;
		AssetManager(const AssetManager&) = delete;
		AssetManager(AssetManager&&) noexcept = delete;
		~AssetManager();

		AssetManager& operator=(const AssetManager&) = delete;
		AssetManager& operator=(AssetManager&&) noexcept = delete;

		template <typename AssetType, typename ConfigType>
		bool Import(const String path, const ConfigType& importConfig);

		template <typename AssetType>
		AssetRef<AssetType> Load(const xg::Guid& guid);

		template <typename AssetType>
		AssetRef<AssetType> Load(const String path); // call guid version

		template <typename AssetType>
		void Unload(const xg::Guid& guid);

		template <typename AssetType>
		void SetPersistent(const xg::Guid& guid);

	private:
		robin_hood::unordered_map<String, xg::Guid> pathGuidTable;

	};

	extern template bool AssetManager::Import<Texture, TextureImportConfig>(const String path, const TextureImportConfig& importConfig);
	extern template bool AssetManager::Import<StaticMesh, StaticMeshImportConfig>(const String path, const StaticMeshImportConfig& importConfig);
	extern template bool AssetManager::Import<Model, ModelImportConfig>(const String path, const ModelImportConfig& importConfig);
	extern template bool AssetManager::Import<Shader, ShaderImportConfig>(const String path, const ShaderImportConfig& importConfig);
	extern template bool AssetManager::Import<Audio, AudioImportConfig>(const String path, const AudioImportConfig& importConfig);
	extern template bool AssetManager::Import<Script, ScriptImportConfig>(const String path, const ScriptImportConfig& importConfig);

} // namespace fe