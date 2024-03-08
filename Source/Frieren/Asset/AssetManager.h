#pragma once
#include <Asset/Common.h>
#include <Asset/Texture.h>

namespace fe
{
	// #wip
	class StaticMesh;
	struct StaticMeshImportConfig;
	class AssetManager final
	{
		friend class RefHandle<Texture, AssetManager*>;
		friend class Handle<Texture, AssetManager*>;

		friend class RefHandle<StaticMesh, AssetManager*>;
		friend class Handle<StaticMesh, AssetManager*>;

	public:
		AssetManager();
		AssetManager(const AssetManager&) = delete;
		AssetManager(AssetManager&&) noexcept = delete;
		~AssetManager();

		AssetManager& operator=(const AssetManager&) = delete;
		AssetManager& operator=(AssetManager&&) noexcept = delete;

		std::optional<xg::Guid> QueryGuid(const String resPathStr) const
		{
			/* Invalid Until properly loaded. */
			const auto itr = resPathGuidTable.find(resPathStr);
			if (itr != resPathGuidTable.cend())
			{
				return itr->second;
			}

			return std::nullopt;
		}

		// std::optional<EAssetType> QueryAssetType(const xg::Guid& guid) const;
		// std::optional<EAssetType> QueryAssetType(const String resPath) const;

		// if config == nullopt ? try query config from metadata : make default config based on resource file
		bool							  ImportTexture(const String resPath, std::optional<TextureImportConfig> config = std::nullopt, const bool bIsPersistent = false);
		RefHandle<Texture, AssetManager*> LoadTexture(const xg::Guid& guid);
		RefHandle<Texture, AssetManager*> LoadTexture(const String resPath);

		// bool				 ImportStaticMeseh(const String resPath, const StaticMeshImportConfig& config);
		// AssetRef<StaticMesh> LoadStaticMesh(const xg::Guid& guid);
		// AssetRef<StaticMesh> LoadStaticMesh(const String resPath)
		//{
		//	const auto guid = QueryGuid(resPath);
		//	return guid ? LoadStaticMesh(*guid) : AssetRef<StaticMesh>{};
		// }

		// 무조건 적인 강제 에셋 해제. 모든 guid-resource 테이블중 임의의 한 테이블에서 guid가 존재하면 unload
		void Unload(const xg::Guid& guid);
		void Unload(const String resPath);
		//{
		//	if (const auto guid = QueryGuid(resPath);
		//		guid)
		//	{
		//		Unload(*guid);
		//	}
		//}

	private:
		template <typename AssetType>
		void operator()(const RefHandle<AssetType, AssetManager*>&, AssetType*)
		{
			// const uint64_t refCount = instance->GetRefCount().fetch_add(-1);
			// if (refCount == 0 && !instance->IsPersistent())
			//{
			//	Unload(instance->GetGuid());
			// }
		}

	private:
		robin_hood::unordered_map<String, xg::Guid>							   resPathGuidTable;
		robin_hood::unordered_map<xg::Guid, Handle<Texture, AssetManager*>>	   guidTextureTable;
		robin_hood::unordered_map<xg::Guid, Handle<StaticMesh, AssetManager*>> guidStaticMeshTable;
	};

	// 파일 경로
	// 리소스: Executable folder의 ./Resources
	// 리소스 메타데이터: 원본 리소스 파일의 경로.metadata
	// 에셋: Executable folder의 ./Assets
	// 에셋 파일: ./Assets/{GUID}.{ASSET_TYPE} ; Asset Type은 EAssetType 요소의 문자열화
	// -> 즉 GUID만 있어도, ./Asset/{GUID}.{ASSET_TYPE} 파일이 존재하고 ./Asset/{GUID}.{metadata}가 존재한다면 Load 가능
	// 한번 로드되면 캐싱, 에셋 메타데이터에 원본 파일의 path도 존재하므로 Resource Path-GUID 테이블에 등록도 가능
	// 에셋 메타데이터: {GUID}.{metadata}

} // namespace fe