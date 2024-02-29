#include <D3D12/RenderDevice.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUView.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <D3D12/RootSignature.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <dxgidebug.h>
#include <Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>

namespace fe
{
	FE_DEFINE_LOG_CATEGORY(DeviceInfo, ELogVerbosiy::Info)
	FE_DEFINE_LOG_CATEGORY(DeviceWarn, ELogVerbosiy::Warning)
	FE_DEFINE_LOG_CATEGORY(DeviceFatal, ELogVerbosiy::Fatal)

	RenderDevice::RenderDevice()
	{
		const bool bIsAcquiredAdapter = AcquireAdapterFromFactory();
		if (bIsAcquiredAdapter)
		{
			LogAdapterInformations();

			if (CreateDevice())
			{
				SetObjectName(device.Get(), "Device");
				SetSeverityLevel();

				CheckSupportedFeatures();

				if (CreateMemoryAllcator())
				{
					CacheDescriptorHandleIncrementSize();
				}
			}
		}
	}

	RenderDevice::~RenderDevice()
	{
		allocator->Release();

		device.Reset();
		adapter.Reset();

#ifdef _DEBUG
		ComPtr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,
										 DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
#endif
	}

	uint32_t RenderDevice::GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const
	{
		switch (type)
		{
			default:
			case EDescriptorHeapType::CBV_SRV_UAV:
				return cbvSrvUavDescriptorHandleIncrementSize;
			case EDescriptorHeapType::Sampler:
				return samplerDescritorHandleIncrementSize;
			case EDescriptorHeapType::DSV:
				return dsvDescriptorHandleIncrementSize;
			case EDescriptorHeapType::RTV:
				return rtvDescriptorHandleIncrementSize;
		}
	}

	bool RenderDevice::AcquireAdapterFromFactory()
	{
		uint32_t factoryCreationFlags = 0;
		{
#if defined(DEBUG) || defined(_DEBUG)
			factoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
			ComPtr<ID3D12Debug5> debugController;
			const bool bDebugControllerAcquired = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			FE_CONDITIONAL_LOG(DeviceFatal, bDebugControllerAcquired, "Failed to get debug controller.");
			if (bDebugControllerAcquired)
			{
				debugController->EnableDebugLayer();
				FE_LOG(DeviceInfo, "D3D12 Debug Layer Enabled.");
	#if defined(ENABLE_GPU_VALIDATION)
				// gpu based validation?
				debugController->SetEnableGPUBasedValidation(true);
				FE_LOG(DeviceInfo, "D3D12 GPU Based Validation Enabled.")''
	#endif
			}
#endif
		}

		ComPtr<IDXGIFactory6> factory;
		const bool bFactoryCreated = SUCCEEDED(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&factory)));
		FE_CONDITIONAL_LOG(DeviceFatal, bFactoryCreated, "Failed to create factory.");

		if (bFactoryCreated)
		{
			const bool bIsAdapterAcquired = SUCCEEDED(
				factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
			FE_CONDITIONAL_LOG(DeviceFatal, bIsAdapterAcquired, "Failed to acquire adapter from factory.");
			return bIsAdapterAcquired;
		}

		return false;
	}

	void RenderDevice::LogAdapterInformations()
	{
		check(adapter);
		DXGI_ADAPTER_DESC adapterDesc;
		adapter->GetDesc(&adapterDesc);
		FE_LOG(DeviceInfo, "----------- The GPU Infos -----------");
		FE_LOG(DeviceInfo, "{}", Narrower(adapterDesc.Description));
		FE_LOG(DeviceInfo, "Vendor ID: {}", adapterDesc.VendorId);
		FE_LOG(DeviceInfo, "Dedicated Video Memory: {}", adapterDesc.DedicatedVideoMemory);
		FE_LOG(DeviceInfo, "Dedicated System Memory: {}", adapterDesc.DedicatedSystemMemory);
		FE_LOG(DeviceInfo, "Shared System Memory: {}", adapterDesc.SharedSystemMemory);
		FE_LOG(DeviceInfo, "-------------------------------------");
	}

	bool RenderDevice::CreateDevice()
	{
		check(adapter);
		constexpr D3D_FEATURE_LEVEL MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_2;
		const bool bIsDeviceCreated = SUCCEEDED(D3D12CreateDevice(adapter.Get(), MinimumFeatureLevel, IID_PPV_ARGS(&device)));
		FE_CONDITIONAL_LOG(DeviceFatal, bIsDeviceCreated, "Failed to create the device from the adapter.");
		check(device);
		return bIsDeviceCreated;
	}

	void RenderDevice::SetSeverityLevel()
	{
		check(device);
#if defined(_DEBUG) || defined(ENABLE_GPU_VALIDATION)
		ComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		}
		else
		{
			FE_LOG(DeviceWarn, "Failed to query a info queue from the device.");
		}
#endif
	}

	void RenderDevice::CheckSupportedFeatures()
	{
		check(device);
		CD3DX12FeatureSupport features;
		features.Init(device.Get());

		bEnhancedBarriersSupported = features.EnhancedBarriersSupported();

		/**
		 * Ray-tracing Tier:
		 * https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md#d3d12_raytracing_tier
		 */
		switch (features.RaytracingTier())
		{
			case D3D12_RAYTRACING_TIER_1_0:
				bRaytracing10Supported = true;
				break;
			case D3D12_RAYTRACING_TIER_1_1:
				bRaytracing11Supported = true;
				break;
		}

		bRaytracing10Supported = bRaytracing11Supported ? true : bRaytracing10Supported;
		bShaderModel66Supported = features.HighestShaderModel() >= D3D_SHADER_MODEL_6_6;

		/**
		 * Resource Binding Tier:
		 * https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html#levels-of-hardware-support Tiled Resource
		 * Tier: https://learn.microsoft.com/en-us/windows/win32/direct3d11/tiled-resources-features-tiers Conservative
		 * Rasterization Tier:
		 * https://learn.microsoft.com/en-us/windows/win32/direct3d12/conservative-rasterization#implementation-details
		 * Resource Heap Tier:
		 * https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_heap_tier
		 */

		/**
		 * D3D12_FEATURE_DATA_D3D12_OPTIONS6
		 * Variable Shading Rate Tier: https://learn.microsoft.com/en-us/windows/win32/direct3d12/vrs#feature-tiers
		 */

		// D3D12_FEATURE_DATA_D3D12_OPTIONS7: Mesh Shader/Sampler Feedback Tier
		// D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS
	}

	void RenderDevice::CacheDescriptorHandleIncrementSize()
	{
		cbvSrvUavDescriptorHandleIncrementSize =
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		samplerDescritorHandleIncrementSize =
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		dsvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		rtvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	bool RenderDevice::CreateMemoryAllcator()
	{
		check(device);
		D3D12MA::ALLOCATOR_DESC desc{};
		desc.pAdapter = adapter.Get();
		desc.pDevice = device.Get();
		const bool bSucceeded = SUCCEEDED(D3D12MA::CreateAllocator(&desc, &allocator));
		FE_CONDITIONAL_LOG(DeviceFatal, bSucceeded, "Failed to create D3D12MA::Allocator.");
		return bSucceeded;
	}

	std::optional<CommandQueue> RenderDevice::CreateCommandQueue(const std::string_view debugName, const EQueueType queueType)
	{
		check(device);

		const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(queueType);
		check(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

		const D3D12_COMMAND_QUEUE_DESC desc = { .Type = cmdListType,
												.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
												.NodeMask = 0 };

		ComPtr<ID3D12CommandQueue> newCmdQueue;
		if (const HRESULT result = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&newCmdQueue));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create command queue. HRESULT: {:#X}", result);
			return {};
		}

		check(newCmdQueue);
		SetObjectName(newCmdQueue.Get(), debugName);
		
		ComPtr<ID3D12Fence> newFence{};
		if (const HRESULT result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&newFence));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create queue sync fence. HRESULT: {:#X}", result);
			return {};
		}

		check(newFence);
		SetObjectName(newFence.Get(), std::format("{} Sync Fence", debugName));

		return CommandQueue{ std::move(newCmdQueue), queueType, std::move(newFence) };
	}

	std::optional<CommandContext> RenderDevice::CreateCommandContext(const std::string_view debugName, const EQueueType targetQueueType)
	{
		check(device);

		const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(targetQueueType);
		check(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

		ComPtr<ID3D12CommandAllocator> newCmdAllocator;
		if (const HRESULT result = device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&newCmdAllocator));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create command allocator. HRESULT: {:#X}", result);
			return {};
		}

		ComPtr<ID3D12GraphicsCommandList7> newCmdList;
		const D3D12_COMMAND_LIST_FLAGS flags = D3D12_COMMAND_LIST_FLAG_NONE;
		if (const HRESULT result = device->CreateCommandList1(0, cmdListType, flags, IID_PPV_ARGS(&newCmdList));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create command list. HRESULT: {:#X}", result);
			return {};
		}

		check(newCmdAllocator);
		check(newCmdList);
		SetObjectName(newCmdList.Get(), debugName);
		return CommandContext{ std::move(newCmdAllocator), std::move(newCmdList), targetQueueType };
	}

	std::optional<RootSignature> RenderDevice::CreateBindlessRootSignature()
	{
		check(device);
		constexpr uint8_t NumReservedConstants = 16;
		const D3D12_ROOT_PARAMETER rootParam{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
			.Constants = {
				.ShaderRegister = 0,
				.RegisterSpace = 0,
				.Num32BitValues = NumReservedConstants },
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
		};

		const D3D12_ROOT_SIGNATURE_DESC desc{ .NumParameters = 1,
											  .pParameters = &rootParam,
											  .NumStaticSamplers = 0,
											  .pStaticSamplers = nullptr,
											  .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
													   D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |
													   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };

		ComPtr<ID3DBlob> errorBlob;
		ComPtr<ID3DBlob> rootSignatureBlob;
		if (const HRESULT result = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf());
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to serialize root signature. HRESULT: {:#X}, Message: {}", result, errorBlob->GetBufferPointer());
			return {};
		}

		ComPtr<ID3D12RootSignature> newRootSignature;
		if (const HRESULT result = device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&newRootSignature));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create root signature. HRESULT: {:#X}", result);
			return {};
		}

		check(newRootSignature);
		return RootSignature{ std::move(newRootSignature) };
	}

	std::optional<PipelineState> RenderDevice::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
	{
		// #todo 파이프라인 상태 객체 캐싱 구현 (해시 기반)
		check(device);

		ComPtr<ID3D12PipelineState> newPipelineState{};

		if (const HRESULT result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPipelineState));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create graphics pipeline state. HRESULT: {:#X}", result);
			return {};
		}

		check(newPipelineState);
		SetObjectName(newPipelineState.Get(), desc.Name);
		return PipelineState{ std::move(newPipelineState), true };
	}

	std::optional<PipelineState> RenderDevice::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
	{
		// #todo 파이프라인 상태 객체 캐싱 구현 (해시 기반)
		check(device);

		ComPtr<ID3D12PipelineState> newPipelineState{};
		if (const HRESULT result = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newPipelineState));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create compute pipeline state. HRESULT: {:#X}", result);
			return {};
		}

		check(newPipelineState);
		SetObjectName(newPipelineState.Get(), desc.Name);
		return PipelineState{ std::move(newPipelineState), false };
	}

	std::optional<DescriptorHeap> RenderDevice::CreateDescriptorHeap(const EDescriptorHeapType descriptorHeapType, const uint32_t numDescriptors)
	{
		check(device);

		const D3D12_DESCRIPTOR_HEAP_TYPE targetDescriptorHeapType = ToNativeDescriptorHeapType(descriptorHeapType);
		check(targetDescriptorHeapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);

		const bool bIsShaderVisibleDescriptorHeap = IsShaderVisibleDescriptorHeapType(descriptorHeapType);

		const D3D12_DESCRIPTOR_HEAP_DESC desc{ .Type = targetDescriptorHeapType,
											   .NumDescriptors = numDescriptors,
											   .Flags = bIsShaderVisibleDescriptorHeap ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
											   .NodeMask = 0 };

		ComPtr<ID3D12DescriptorHeap> newDescriptorHeap;
		if (const HRESULT result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeap));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create descriptor heap. HRESULT: {:#X}", result);
			return {};
		}

		check(newDescriptorHeap);
		return DescriptorHeap{
			descriptorHeapType,
			std::move(newDescriptorHeap),
			bIsShaderVisibleDescriptorHeap,
			numDescriptors, GetDescriptorHandleIncrementSize(descriptorHeapType)
		};
	}

	std::optional<GpuBuffer> RenderDevice::CreateBuffer(const GpuBufferDesc& bufferDesc)
	{
		check(device);
		check(allocator);

		const D3D12MA::ALLOCATION_DESC allocationDesc = bufferDesc.ToAllocationDesc();
		ComPtr<D3D12MA::Allocation> allocation{};
		ComPtr<ID3D12Resource> resource{};
		if (const HRESULT result = allocator->CreateResource3(
				&allocationDesc, &bufferDesc,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				nullptr, 0, nullptr,
				allocation.GetAddressOf(), IID_PPV_ARGS(&resource));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create buffer. HRESULT: {:#X}", result);
			return {};
		}

		check(allocation);
		check(resource);
		SetObjectName(resource.Get(), bufferDesc.DebugName);
		return GpuBuffer{ bufferDesc, std::move(allocation), std::move(resource) };
	}

	std::optional<GpuTexture> RenderDevice::CreateTexture(const GPUTextureDesc& textureDesc)
	{
		check(device);
		check(allocator);

		const D3D12MA::ALLOCATION_DESC allocationDesc = textureDesc.ToAllocationDesc();
		std::optional<D3D12_CLEAR_VALUE> clearValue{};
		if (textureDesc.IsRenderTargetCompatible())
		{
			clearValue = D3D12_CLEAR_VALUE{
				.Format = textureDesc.Format,
				.Color = { 0.f, 0.f, 0.f, 1.f }
			};
		}
		else if (textureDesc.IsDepthStencilCompatible())
		{
			clearValue = D3D12_CLEAR_VALUE{
				.Format = textureDesc.Format,
				.DepthStencil = { .Depth = 1.f, .Stencil = 0 }
			};
		}

		ComPtr<D3D12MA::Allocation> allocation{};
		ComPtr<ID3D12Resource> resource{};
		if (const HRESULT result = allocator->CreateResource3(&allocationDesc, &textureDesc,
															  D3D12_BARRIER_LAYOUT_UNDEFINED,
															  clearValue ? &clearValue.value() : nullptr,
															  0, nullptr,
															  allocation.GetAddressOf(), IID_PPV_ARGS(&resource));
			!SUCCEEDED(result))
		{
			return {};
		}

		check(allocation);
		check(resource);
		SetObjectName(resource.Get(), textureDesc.DebugName);
		return GpuTexture{ textureDesc, std::move(allocation), std::move(resource) };
	}

	ComPtr<D3D12MA::Pool> RenderDevice::CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc)
	{
		check(device);
		check(allocator != nullptr);

		ComPtr<D3D12MA::Pool> customPool;
		if (const HRESULT result = allocator->CreatePool(&desc, &customPool);
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create custom gpu memory pool.");
			return {};
		}

		return customPool;
	}

	void RenderDevice::UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer)
	{
		check(gpuView.Type == EGpuViewType::ConstantBufferView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GpuBufferDesc& desc = buffer.GetDesc();
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> cbvDesc = desc.ToConstantBufferViewDesc(buffer.GetNative().GetGPUVirtualAddress());
		if (cbvDesc)
		{
			device->CreateConstantBufferView(&cbvDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes)
	{
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		check((offset + sizeInBytes) < buffer.GetDesc().GetSizeAsBytes());
		if (gpuView.Type == EGpuViewType::ConstantBufferView)
		{
			const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
				.BufferLocation = buffer.GetNative().GetGPUVirtualAddress() + offset,
				.SizeInBytes = static_cast<uint32_t>(sizeInBytes)
			};
			device->CreateConstantBufferView(&cbvDesc, gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer)
	{
		check(gpuView.Type == EGpuViewType::ShaderResourceView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GpuBufferDesc& desc = buffer.GetDesc();
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc = desc.ToShaderResourceViewDesc();
		if (srvDesc)
		{
			device->CreateShaderResourceView(&buffer.GetNative(), &srvDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer)
	{
		check(gpuView.Type == EGpuViewType::UnorderedAccessView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GpuBufferDesc& desc = buffer.GetDesc();
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc = desc.ToUnorderedAccessViewDesc();

		if (uavDesc)
		{
			device->CreateUnorderedAccessView(&buffer.GetNative(), nullptr, &uavDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateShaderResourceView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource)
	{
		check(gpuView.Type == EGpuViewType::ShaderResourceView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc& desc = texture.GetDesc();
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc = desc.ToShaderResourceViewDesc(subresource);

		if (srvDesc)
		{
			device->CreateShaderResourceView(&texture.GetNative(), &srvDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateUnorderedAccessView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource)
	{
		check(gpuView.Type == EGpuViewType::UnorderedAccessView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc& desc = texture.GetDesc();
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc = desc.ToUnorderedAccessViewDesc(subresource);

		if (uavDesc)
		{
			device->CreateUnorderedAccessView(&texture.GetNative(), nullptr, &uavDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateRenderTargetView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource)
	{
		check(gpuView.Type == EGpuViewType::RenderTargetView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc& desc = texture.GetDesc();
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC> rtvDesc = desc.ToRenderTargetViewDesc(subresource);

		if (rtvDesc)
		{
			device->CreateRenderTargetView(&texture.GetNative(), &rtvDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

	void RenderDevice::UpdateDepthStencilView(const GpuView& gpuView, GpuTexture& texture, const GpuViewTextureSubresource& subresource)
	{
		check(gpuView.Type == EGpuViewType::DepthStencilView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc& desc = texture.GetDesc();
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> dsvDesc = desc.ToDepthStencilViewDesc(subresource);

		if (dsvDesc)
		{
			device->CreateDepthStencilView(&texture.GetNative(), &dsvDesc.value(), gpuView.CPUHandle);
		}
		else
		{
			checkNoEntry();
		}
	}

} // namespace fe