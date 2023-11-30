#pragma once
#include <AgilitySDK/d3dcommon.h>
#include <AgilitySDK/d3d12.h>
#include <AgilitySDK/d3dx12/d3dx12.h>
#include <AgilitySDK/d3d12sdklayers.h>
#include <AgilitySDK/d3d12shader.h>
#include <AgilitySDK/dxgiformat.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <directx-dxc/dxcapi.h>
#include <D3D12MemAlloc.h>

namespace fe::Private
{
	void SetD3DObjectName(ID3D12Object* object, std::string_view name);
} // namespace fe::Private

namespace fe::wrl
{
	template <typename Ty>
	using ComPtr = Microsoft::WRL::ComPtr<Ty>;
}
