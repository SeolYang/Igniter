#pragma once
#include <AgilitySDK/d3dcommon.h>

#pragma warning(push)
#pragma warning(disable : 26827)
#include <AgilitySDK/d3d12.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 6001)
#pragma warning(disable : 26819)
#include <AgilitySDK/d3dx12/d3dx12.h>
#include <AgilitySDK/d3d12sdklayers.h>
#pragma warning(pop)

#include <AgilitySDK/d3d12shader.h>
#include <AgilitySDK/dxgiformat.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <directx-dxc/dxcapi.h>

#pragma warning(push)
#pragma warning(disable : 26827)
#pragma warning(disable : 26495)
#pragma warning(disable : 4100)
#pragma warning(disable : 4189)
#pragma warning(disable : 4324)
#pragma warning(disable : 4505)
#include <D3D12MemAlloc.h>
#pragma warning(pop)

namespace fe::dx
{
	template <typename Ty>
	using ComPtr = Microsoft::WRL::ComPtr<Ty>;

	void SetObjectName(ID3D12Object* object, std::string_view name);
} // namespace fe::dx