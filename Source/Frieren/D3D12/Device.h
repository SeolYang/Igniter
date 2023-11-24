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

namespace fe
{
	inline bool IsDXCallSucceed(const HRESULT result)
	{
		return result == S_OK;
	}

	class Device
	{
	public:
		Device();
		~Device();

		ID3D12CommandQueue& GetDirectQueue() const { return *directQueue.Get(); }
		ID3D12CommandQueue& GetAsyncComputeQueue() const { return *asyncComputeQueue.Get(); }
		ID3D12CommandQueue& GetCopyQueue() const { return *copyQueue.Get(); }

		void FlushQueue(D3D12_COMMAND_LIST_TYPE queueType);
		void FlushGPU();

	private:
		bool AcquireAdapterFromFactory();
		void LogAdapterInformations();
		bool CreateDevice();
		void SetSeverityLevel();
		void CheckSupportedFeatures();
		bool CreateCommandQueues();

	private:
		wrl::ComPtr<IDXGIAdapter>  adapter;
		wrl::ComPtr<ID3D12Device9> device;

		wrl::ComPtr<ID3D12CommandQueue> directQueue;
		wrl::ComPtr<ID3D12CommandQueue> asyncComputeQueue;
		wrl::ComPtr<ID3D12CommandQueue> copyQueue;
	};
} // namespace fe