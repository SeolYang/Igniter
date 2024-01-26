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
#include <exception>

namespace fe
{
	// https://github.com/Microsoft/DirectXTK/wiki/ThrowIfFailed
	class ComException : public std::exception
	{
	public:
		ComException(const HRESULT hr)
			: result(hr)
		{
		}

		const char* what() const noexcept override
		{
			static char strBuffer[64] = {};
			sprintf_s(strBuffer, "Failure with HRESULT of %08X",
				static_cast<unsigned int>(result));
			return strBuffer;
		}

	private:
		HRESULT result;
	};

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (!SUCCEEDED(hr))
		{
			throw ComException(hr);
		}
	}

} // namespace fe

namespace fe::Private
{
	void SetD3DObjectName(ID3D12Object* object, std::string_view name);
} // namespace fe::Private

namespace fe::wrl
{
	template <typename Ty>
	using ComPtr = Microsoft::WRL::ComPtr<Ty>;
}
