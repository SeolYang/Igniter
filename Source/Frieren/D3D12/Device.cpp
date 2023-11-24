#include <dxgidebug.h>

#include <D3D12/Device.h>
#include <Core/Assert.h>
#include <Engine.h>

extern "C"
{
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
}
extern "C"
{
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

namespace fe::Private
{
	void SetD3DObjectName(ID3D12Object* object, std::string_view name)
	{
		if (object != nullptr)
		{
			object->SetName(Wider(name).c_str());
		}
	}
} // namespace fe::Private

namespace fe
{
	Device::Device()
	{
		const bool bIsAcquiredAdapter = AcquireAdapterFromFactory();
		if (bIsAcquiredAdapter)
		{
			FE_LOG(D3D12Info, "The Adapter successfully acuiqred from factory.");
			LogAdapterInformations();

			const bool bIsDeviceCreated = CreateDevice();
			if (bIsDeviceCreated)
			{
				FE_LOG(D3D12Info, "The Device successfully created from the adapter.");
				Private::SetD3DObjectName(device.Get(), "Device");
				SetSeverityLevel();

				const bool bIsCommandQueuesCreated = CreateCommandQueues();
				if (bIsCommandQueuesCreated)
				{
					FE_LOG(D3D12Info, "Command Queues are successfully created.");
					Private::SetD3DObjectName(directQueue.Get(), "DirectQueue");
					Private::SetD3DObjectName(asyncComputeQueue.Get(), "AsyncComputeQueue");
					Private::SetD3DObjectName(copyQueue.Get(), "CopyQueue");
				}
			}
		}
	}

	Device::~Device()
	{
		FlushGPU();

		copyQueue.Reset();
		asyncComputeQueue.Reset();
		directQueue.Reset();

		device.Reset();
		adapter.Reset();

#ifdef _DEBUG
		wrl::ComPtr<IDXGIDebug1> dxgiDebug;
		if (IsDXCallSucceeded(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
#endif
	}

	void Device::FlushQueue(const D3D12_COMMAND_LIST_TYPE queueType)
	{
		FE_ASSERT_LOG(D3D12Fatal, queueType != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Invalid queue type to flush.");
		FE_ASSERT_LOG(D3D12Fatal, queueType != D3D12_COMMAND_LIST_TYPE_NONE, "Invalid queue type to flush.");
		FE_ASSERT_LOG(D3D12Fatal, queueType != D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE, "Invalid queue type to flush.");
		FE_ASSERT_LOG(D3D12Fatal, queueType != D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE, "Invalid queue type to flush.");
		FE_ASSERT_LOG(D3D12Fatal, queueType != D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS, "Invalid queue type to flush.");

		wrl::ComPtr<ID3D12Fence> fence;
		if (IsDXCallSucceeded(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		{
			ID3D12CommandQueue* queue = nullptr;
			switch (queueType)
			{
				case D3D12_COMMAND_LIST_TYPE_DIRECT:
					queue = &GetDirectQueue();
					break;
				case D3D12_COMMAND_LIST_TYPE_COMPUTE:
					queue = &GetAsyncComputeQueue();
					break;
				case D3D12_COMMAND_LIST_TYPE_COPY:
					queue = &GetCopyQueue();
					break;
			}

			FE_ASSERT(queue != nullptr, "Invalid queue.");

			if (IsDXCallSucceeded(queue->Signal(fence.Get(), 1)))
			{
				const HANDLE handleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				fence->SetEventOnCompletion(1, handleFenceEvent);
				WaitForSingleObject(handleFenceEvent, INFINITE);
				CloseHandle(handleFenceEvent);
			}
			else
			{
				FE_FORCE_ASSERT_LOG(D3D12Fatal, "Failed to signal fence.");
			}
		}
		else
		{
			FE_FORCE_ASSERT_LOG(D3D12Fatal, "Failed to create fence for flush.");
		}
	}

	void Device::FlushGPU()
	{
		FlushQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		FlushQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
		FlushQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	}

	bool Device::AcquireAdapterFromFactory()
	{
		uint32_t factoryCreationFlags = 0;
		{
#if defined(DEBUG) || defined(_DEBUG)
			factoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
			wrl::ComPtr<ID3D12Debug5> debugController;
			const bool				  bDebugControllerAcquired = IsDXCallSucceeded(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			FE_ASSERT_LOG(fe::D3D12Fatal, bDebugControllerAcquired, "Failed to get debug controller.");
			if (bDebugControllerAcquired)
			{
				debugController->EnableDebugLayer();
				FE_LOG(D3D12Info, "D3D12 Debug Layer Enabled.");
	#if defined(ENABLE_GPU_VALIDATION)
				// gpu based validation?
				debugController->SetEnableGPUBasedValidation(true);
				FE_LOG(D3D12Info, "D3D12 GPU Based Validation Enabled.")''
	#endif
			}
#endif
		}

		wrl::ComPtr<IDXGIFactory6> factory;
		const bool				   bFactoryCreated = IsDXCallSucceeded(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&factory)));
		FE_ASSERT_LOG(D3D12Fatal, bFactoryCreated, "Failed to create factory.");

		if (bFactoryCreated)
		{
			const bool bIsAdapterAcquired = IsDXCallSucceeded(factory->EnumAdapterByGpuPreference(
				0,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(&adapter)));
			FE_ASSERT_LOG(D3D12Fatal, bIsAdapterAcquired, "Failed to acquire adapter from factory.");
			return bIsAdapterAcquired;
		}

		return false;
	}

	void Device::LogAdapterInformations()
	{
		DXGI_ADAPTER_DESC adapterDesc;
		adapter->GetDesc(&adapterDesc);
		FE_LOG(D3D12Info, "----------- The GPU Infos -----------");
		FE_LOG(D3D12Info, "{}", Narrower(adapterDesc.Description));
		FE_LOG(D3D12Info, "Vendor ID: {}", adapterDesc.VendorId);
		FE_LOG(D3D12Info, "Dedicated Video Memory: {}", adapterDesc.DedicatedVideoMemory);
		FE_LOG(D3D12Info, "Dedicated System Memory: {}", adapterDesc.DedicatedSystemMemory);
		FE_LOG(D3D12Info, "Shared System Memory: {}", adapterDesc.SharedSystemMemory);
		FE_LOG(D3D12Info, "-------------------------------------");
	}

	bool Device::CreateDevice()
	{
		constexpr D3D_FEATURE_LEVEL MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_2;
		const bool					bIsDeviceCreated = IsDXCallSucceeded(D3D12CreateDevice(adapter.Get(), MinimumFeatureLevel, IID_PPV_ARGS(&device)));
		FE_ASSERT_LOG(D3D12Fatal, bIsDeviceCreated, "Failed to create the device from the adapter.");
		return bIsDeviceCreated;
	}

	void Device::SetSeverityLevel()
	{
#if defined(_DEBUG) || defined(ENABLE_GPU_VALIDATION)
		wrl::ComPtr<ID3D12InfoQueue> infoQueue;
		if (IsDXCallSucceeded(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		}
		else
		{
			FE_LOG(D3D12Warn, "Failed to query a info queue from the device.");
		}
#endif
	}

	void Device::CheckSupportedFeatures()
	{
		// @todo impl proper tier/feature check
		/**
		 * D3D12_FEATURE_DATA_D3D12_OPTIONS
		 * Resource Binding Tier: https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#levels-of-hardware-support
		 * Tiled Resource Tier: https://learn.microsoft.com/en-us/windows/win32/direct3d11/tiled-resources-features-tiers
		 * Conservative Rasterization Tier: https://learn.microsoft.com/en-us/windows/win32/direct3d12/conservative-rasterization#implementation-details
		 * Resource Heap Tier: https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier
		 */

		/**
		 * D3D12_FEATURE_DATA_D3D12_OPTIONS5
		 * Ray-tracing Tier: https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md#d3d12_raytracing_tier
		 */

		/**
		 * D3D12_FEATURE_DATA_D3D12_OPTIONS6
		 * Variable Shading Rate Tier: https://learn.microsoft.com/en-us/windows/win32/direct3d12/vrs#feature-tiers
		 */

		// D3D12_FEATURE_DATA_D3D12_OPTIONS7: Mesh Shader/Sampler Feedback Tier
		// D3D12_FEATURE_DATA_SHADER_MODEL: Shader Model > 6.6
		// D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS
	}

	bool Device::CreateCommandQueues()
	{
		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.NodeMask = 0;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			if (!IsDXCallSucceeded(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&directQueue))))
			{
				FE_ASSERT_LOG(D3D12Fatal, false, "Failed to create the direct command queue.");
				return false;
			}
		}

		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.NodeMask = 0;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			if (!IsDXCallSucceeded(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&asyncComputeQueue))))
			{
				FE_ASSERT_LOG(D3D12Fatal, false, "Failed to create the async compute command queue.");
				return false;
			}
		}

		{
			D3D12_COMMAND_QUEUE_DESC desc = {};
			desc.NodeMask = 0;
			desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
			if (!IsDXCallSucceeded(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&copyQueue))))
			{
				FE_ASSERT_LOG(D3D12Fatal, false, "Failed to create the copy command queue.");
				return false;
			}
		}

		return true;
	}

} // namespace fe