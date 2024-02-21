#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/GPUView.h>
#include <D3D12/Fence.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <D3D12/RootSignature.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>
#include <dxgidebug.h>
#include <Engine.h>
#include <Core/Assert.h>
#include <Core/Log.h>

namespace fe::dx
{
	FE_DECLARE_LOG_CATEGORY(DeviceInfo, ELogVerbosiy::Info)
	FE_DECLARE_LOG_CATEGORY(DeviceWarn, ELogVerbosiy::Warning)
	FE_DECLARE_LOG_CATEGORY(DeviceFatal, ELogVerbosiy::Fatal)

	Device::Device()
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
					CreateBindlessDescriptorHeaps();
				}
			}
		}
	}

	Device::~Device()
	{
		cbvSrvUavDescriptorHeap.reset();
		samplerDescriptorHeap.reset();
		rtvDescriptorHeap.reset();
		dsvDescriptorHeap.reset();

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

	std::array<std::reference_wrapper<DescriptorHeap>, 2> Device::GetBindlessDescriptorHeaps()
	{
		return { *cbvSrvUavDescriptorHeap, *samplerDescriptorHeap };
	}

	uint32_t Device::GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const
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

	bool Device::AcquireAdapterFromFactory()
	{
		uint32_t factoryCreationFlags = 0;
		{
#if defined(DEBUG) || defined(_DEBUG)
			factoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
			ComPtr<ID3D12Debug5> debugController;
			const bool			 bDebugControllerAcquired = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
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
		const bool			  bFactoryCreated = SUCCEEDED(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&factory)));
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

	void Device::LogAdapterInformations()
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

	bool Device::CreateDevice()
	{
		check(adapter);
		constexpr D3D_FEATURE_LEVEL MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_2;
		const bool					bIsDeviceCreated = SUCCEEDED(D3D12CreateDevice(adapter.Get(), MinimumFeatureLevel, IID_PPV_ARGS(&device)));
		FE_CONDITIONAL_LOG(DeviceFatal, bIsDeviceCreated, "Failed to create the device from the adapter.");
		check(device);
		return bIsDeviceCreated;
	}

	void Device::SetSeverityLevel()
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

	void Device::CheckSupportedFeatures()
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

	void Device::CacheDescriptorHandleIncrementSize()
	{
		cbvSrvUavDescriptorHandleIncrementSize =
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		samplerDescritorHandleIncrementSize =
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		dsvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		rtvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	bool Device::CreateMemoryAllcator()
	{
		check(device);
		D3D12MA::ALLOCATOR_DESC desc{};
		desc.pAdapter = adapter.Get();
		desc.pDevice = device.Get();
		const bool bSucceeded = SUCCEEDED(D3D12MA::CreateAllocator(&desc, &allocator));
		FE_CONDITIONAL_LOG(DeviceFatal, bSucceeded, "Failed to create D3D12MA::Allocator.");
		return bSucceeded;
	}

	void Device::CreateBindlessDescriptorHeaps()
	{
		cbvSrvUavDescriptorHeap = std::make_unique<DescriptorHeap>(
			CreateDescriptorHeap(EDescriptorHeapType::CBV_SRV_UAV, NumCbvSrvUavDescriptors).value());
		samplerDescriptorHeap = std::make_unique<DescriptorHeap>(
			CreateDescriptorHeap(EDescriptorHeapType::Sampler, NumSamplerDescriptors).value());
		rtvDescriptorHeap =
			std::make_unique<DescriptorHeap>(CreateDescriptorHeap(EDescriptorHeapType::RTV, NumRtvDescriptors).value());
		dsvDescriptorHeap =
			std::make_unique<DescriptorHeap>(CreateDescriptorHeap(EDescriptorHeapType::DSV, NumDsvDescriptors).value());
	}

	std::optional<Fence> Device::CreateFence(const std::string_view debugName, const uint64_t initialCounter /*= 0*/)
	{
		check(device);

		ComPtr<ID3D12Fence> newFence{};
		if (const HRESULT result = device->CreateFence(initialCounter, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&newFence));
			!SUCCEEDED(result))
		{
			// #todo 가능하다면, HRESULT -> String 으로 변환 구현
			FE_LOG(DeviceFatal, "Failed to create fence. HRESULT: {}", result);
			return {};
		}

		check(newFence);
		SetObjectName(newFence.Get(), debugName);

		return Fence{ std::move(newFence) };
	}

	std::optional<CommandQueue> Device::CreateCommandQueue(const EQueueType queueType)
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
			FE_LOG(DeviceFatal, "Failed to create command queue. HRESULT: {}", result);
			return {};
		}

		check(newCmdQueue);
		return CommandQueue{ std::move(newCmdQueue), queueType };
	}

	std::optional<CommandContext> Device::CreateCommandContext(const EQueueType targetQueueType)
	{
		check(device);

		const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(targetQueueType);
		check(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

		ComPtr<ID3D12CommandAllocator> newCmdAllocator;
		if (const HRESULT result = device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&newCmdAllocator));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create command allocator. HRESULT: {}", result);
			return {};
		}

		ComPtr<ID3D12GraphicsCommandList7> newCmdList;
		const D3D12_COMMAND_LIST_FLAGS	   flags = D3D12_COMMAND_LIST_FLAG_NONE;
		if (const HRESULT result = device->CreateCommandList1(0, cmdListType, flags, IID_PPV_ARGS(&newCmdList));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create command list. HRESULT: {}", result);
			return {};
		}

		check(newCmdAllocator);
		check(newCmdList);
		return CommandContext{ std::move(newCmdAllocator), std::move(newCmdList), targetQueueType };
	}

	std::optional<RootSignature> Device::CreateBindlessRootSignature()
	{
		check(device);

		// WITH THOSE FLAGS, IT MUST BIND DESCRIPTOR HEAP FIRST BEFORE BINDING ROOT SIGNATURE
		/*
		  D3D12_DESCRIPTOR_RANGE			texture2DRange{};
		  texture2DRange.BaseShaderRegister = 0;
		  texture2DRange.NumDescriptors = 1024;
		  texture2DRange.OffsetInDescriptorsFromTableStart = 0;
		  texture2DRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		  texture2DRange.RegisterSpace = 0;

		  D3D12_ROOT_PARAMETER texture2DTable{};
		  texture2DTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		  texture2DTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		  texture2DTable.DescriptorTable.NumDescriptorRanges = 1;
		  texture2DTable.DescriptorTable.pDescriptorRanges = &texture2DRange;
		*/

		const D3D12_ROOT_SIGNATURE_DESC desc{ .NumParameters = 0,
											  .pParameters = nullptr,
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
			FE_LOG(DeviceFatal, "Failed to serialize root signature. HRESULT: {}, Message: {}", result, errorBlob->GetBufferPointer());
			return {};
		}

		ComPtr<ID3D12RootSignature> newRootSignature;
		if (const HRESULT result = device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&newRootSignature));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create root signature. HRESULT: {}", result);
			return {};
		}

		check(newRootSignature);
		return RootSignature{ std::move(newRootSignature) };
	}

	std::optional<PipelineState> Device::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
	{
		// #todo 파이프라인 상태 객체 캐싱 구현 (해시 기반)
		check(device);

		ComPtr<ID3D12PipelineState> newPipelineState{};

		if (const HRESULT result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPipelineState));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create graphics pipeline state. HRESULT: {}", result);
			return {};
		}

		check(newPipelineState);
		SetObjectName(newPipelineState.Get(), desc.Name);

		return PipelineState{ std::move(newPipelineState), true };
	}

	std::optional<PipelineState> Device::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
	{
		// #todo 파이프라인 상태 객체 캐싱 구현 (해시 기반)
		check(device);

		ComPtr<ID3D12PipelineState> newPipelineState{};
		if (const HRESULT result = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newPipelineState));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create compute pipeline state. HRESULT: {}", result);
			return {};
		}

		check(newPipelineState);
		SetObjectName(newPipelineState.Get(), desc.Name);

		return PipelineState{ std::move(newPipelineState), false };
	}

	std::optional<DescriptorHeap> Device::CreateDescriptorHeap(const EDescriptorHeapType descriptorHeapType, const uint32_t numDescriptors)
	{
		check(device);

		const D3D12_DESCRIPTOR_HEAP_TYPE targetDescriptorHeapType = ToNativeDescriptorHeapType(descriptorHeapType);
		check(targetDescriptorHeapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);

		const bool bIsShaderVisibleDescriptorHeap = IsShaderVisibleDescriptorHeapType(descriptorHeapType);

		const D3D12_DESCRIPTOR_HEAP_DESC desc{ .Type = targetDescriptorHeapType,
											   .NumDescriptors = numDescriptors,
											   .Flags = bIsShaderVisibleDescriptorHeap
															? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
															: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
											   .NodeMask = 0 };

		ComPtr<ID3D12DescriptorHeap> newDescriptorHeap;
		if (const HRESULT result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeap));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create descriptor heap. HRESULT: {}", result);
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

	std::optional<GPUBuffer> Device::CreateBuffer(const GPUBufferDesc& bufferDesc)
	{
		check(device);
		check(allocator);

		const D3D12MA::ALLOCATION_DESC allocationDesc = bufferDesc.ToAllocationDesc();
		ComPtr<D3D12MA::Allocation>	   allocation{};
		ComPtr<ID3D12Resource>		   resource{};
		if (const HRESULT result = allocator->CreateResource3(
				&allocationDesc, &bufferDesc,
				D3D12_BARRIER_LAYOUT_UNDEFINED,
				nullptr, 0, nullptr,
				allocation.GetAddressOf(), IID_PPV_ARGS(&resource));
			!SUCCEEDED(result))
		{
			FE_LOG(DeviceFatal, "Failed to create buffer. HRESULT");
			return {};
		}

		check(allocation);
		check(resource);
		return GPUBuffer{ bufferDesc, std::move(allocation), std::move(resource) };
	}

	std::optional<GPUTexture> Device::CreateTexture(const GPUTextureDesc& textureDesc)
	{
		check(device);
		check(allocator);

		const D3D12MA::ALLOCATION_DESC	 allocationDesc = textureDesc.ToAllocationDesc();
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
		ComPtr<ID3D12Resource>		resource{};
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

		return GPUTexture{ textureDesc, std::move(allocation), std::move(resource) };
	}

	FrameResource<GPUView> Device::CreateConstantBufferView(DeferredDeallocator& deferredDeallocator, GPUBuffer& gpuBuffer)
	{
		check(gpuBuffer);
		const GPUBufferDesc&						   desc = gpuBuffer.GetDesc();
		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> cbvDesc = desc.ToConstantBufferViewDesc(gpuBuffer.GetNative().GetGPUVirtualAddress());

		FrameResource<GPUView> newCbv{};
		if (cbvDesc)
		{
			check(cbvSrvUavDescriptorHeap);
			newCbv = cbvSrvUavDescriptorHeap->Request(deferredDeallocator, EDescriptorType::ConstantBufferView);

			check(newCbv && newCbv->IsValid());
			check(newCbv->HasValidCPUHandle());
			check(newCbv->Type == EDescriptorType::ConstantBufferView);
			device->CreateConstantBufferView(&cbvDesc.value(), newCbv->CPUHandle);
		}

		check(newCbv);
		return newCbv;
	}

	FrameResource<GPUView> Device::CreateShaderResourceView(DeferredDeallocator& deferredDeallocator, GPUBuffer& gpuBuffer)
	{
		check(gpuBuffer);
		const GPUBufferDesc&						   desc = gpuBuffer.GetDesc();
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc = desc.ToShaderResourceViewDesc();

		FrameResource<GPUView> newSrv{};
		if (srvDesc)
		{
			check(cbvSrvUavDescriptorHeap);
			newSrv = cbvSrvUavDescriptorHeap->Request(deferredDeallocator, EDescriptorType::ShaderResourceView);

			check(newSrv && newSrv->IsValid());
			check(newSrv->HasValidCPUHandle());
			check(newSrv->Type == EDescriptorType::ShaderResourceView);
			device->CreateShaderResourceView(&gpuBuffer.GetNative(), &srvDesc.value(), newSrv->CPUHandle);
		}

		return newSrv;
	}

	FrameResource<GPUView> Device::CreateUnorderedAccessView(DeferredDeallocator& deferredDeallocator, GPUBuffer& gpuBuffer)
	{
		check(gpuBuffer);
		const GPUBufferDesc&							desc = gpuBuffer.GetDesc();
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc = desc.ToUnorderedAccessViewDesc();

		FrameResource<GPUView> newUav{};
		if (uavDesc)
		{
			check(cbvSrvUavDescriptorHeap);
			newUav = cbvSrvUavDescriptorHeap->Request(deferredDeallocator, EDescriptorType::UnorderedAccessView);

			check(newUav && newUav->IsValid());
			check(newUav->HasValidCPUHandle());
			check(newUav->Type == EDescriptorType::UnorderedAccessView);
			device->CreateUnorderedAccessView(&gpuBuffer.GetNative(), nullptr, &uavDesc.value(), newUav->CPUHandle);
		}

		return newUav;
	}

	FrameResource<GPUView> Device::CreateShaderResourceView(DeferredDeallocator& deferredDeallocator, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		check(gpuTexture);
		const GPUTextureDesc&						   desc = gpuTexture.GetDesc();
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc = desc.ToShaderResourceViewDesc(subresource);

		FrameResource<GPUView> newSrv{};
		if (srvDesc)
		{
			check(cbvSrvUavDescriptorHeap);
			newSrv = cbvSrvUavDescriptorHeap->Request(deferredDeallocator, EDescriptorType::ShaderResourceView);

			check(newSrv && newSrv->IsValid());
			check(newSrv->HasValidCPUHandle());
			check(newSrv->Type == EDescriptorType::ShaderResourceView);
			device->CreateShaderResourceView(&gpuTexture.GetNative(), &srvDesc.value(), newSrv->CPUHandle);
		}

		return newSrv;
	}

	FrameResource<GPUView> Device::CreateUnorderedAccessView(DeferredDeallocator& deferredDeallocator, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		check(gpuTexture);
		const GPUTextureDesc&							desc = gpuTexture.GetDesc();
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc = desc.ToUnorderedAccessViewDesc(subresource);

		FrameResource<GPUView> newUav{};
		if (uavDesc)
		{
			check(cbvSrvUavDescriptorHeap);
			newUav = cbvSrvUavDescriptorHeap->Request(deferredDeallocator, EDescriptorType::UnorderedAccessView);

			check(newUav && newUav->IsValid());
			check(newUav->HasValidCPUHandle());
			check(newUav->Type == EDescriptorType::UnorderedAccessView);
			device->CreateUnorderedAccessView(&gpuTexture.GetNative(), nullptr, &uavDesc.value(), newUav->CPUHandle);
		}

		return newUav;
	}

	FrameResource<GPUView> Device::CreateRenderTargetView(DeferredDeallocator& deferredDeallocator, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		check(gpuTexture);
		const GPUTextureDesc&						 desc = gpuTexture.GetDesc();
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC> rtvDesc = desc.ToRenderTargetViewDesc(subresource);

		FrameResource<GPUView> newRtv{};
		if (rtvDesc)
		{
			check(rtvDescriptorHeap);
			newRtv = rtvDescriptorHeap->Request(deferredDeallocator, EDescriptorType::RenderTargetView);

			check(newRtv && newRtv->IsValid());
			check(newRtv->HasValidCPUHandle());
			check(newRtv->Type == EDescriptorType::RenderTargetView);
			device->CreateRenderTargetView(&gpuTexture.GetNative(), &rtvDesc.value(), newRtv->CPUHandle);
		}

		return newRtv;
	}

	FrameResource<GPUView> Device::CreateDepthStencilView(DeferredDeallocator& deferredDeallocator, GPUTexture& gpuTexture, const GPUTextureSubresource& subresource)
	{
		check(gpuTexture);
		const GPUTextureDesc&						 desc = gpuTexture.GetDesc();
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> dsvDesc = desc.ToDepthStencilViewDesc(subresource);

		FrameResource<GPUView> newDsv{};
		if (dsvDesc)
		{
			check(dsvDescriptorHeap);
			newDsv = dsvDescriptorHeap->Request(deferredDeallocator, EDescriptorType::DepthStencilView);

			check(newDsv && newDsv->IsValid());
			check(newDsv->HasValidCPUHandle());
			check(newDsv->Type == EDescriptorType::DepthStencilView);
			device->CreateDepthStencilView(&gpuTexture.GetNative(), &dsvDesc.value(), newDsv->CPUHandle);
		}

		return newDsv;
	}

} // namespace fe::dx