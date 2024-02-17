#include <dxgidebug.h>
#include <Core/Assert.h>
#include <Engine.h>
#include <D3D12/Device.h>
#include <D3D12/DescriptorHeap.h>
#include <D3D12/GPUBufferDesc.h>
#include <D3D12/GPUBuffer.h>
#include <D3D12/GPUTextureDesc.h>
#include <D3D12/GPUTexture.h>
#include <D3D12/Fence.h>
#include <D3D12/PipelineState.h>
#include <D3D12/PipelineStateDesc.h>
#include <D3D12/RootSignature.h>
#include <D3D12/CommandQueue.h>
#include <D3D12/CommandContext.h>

namespace fe::dx
{
	Device::Device()
	{
		const bool bIsAcquiredAdapter = AcquireAdapterFromFactory();
		if (bIsAcquiredAdapter)
		{
			FE_LOG(D3D12Info, "The Adapter successfully acuiqred from factory.");
			LogAdapterInformations();

			if (CreateDevice())
			{
				FE_LOG(D3D12Info, "The Device successfully created from the adapter.");
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

	std::optional<Descriptor> Device::CreateShaderResourceView(GPUBuffer& gpuBuffer)
	{
		const GPUBufferDesc&						   desc = gpuBuffer.GetDesc();
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc = desc.ToShaderResourceViewDesc();
		if (srvDesc)
		{
			std::optional<Descriptor> descriptor =
				cbvSrvUavDescriptorHeap->Allocate(EDescriptorType::ShaderResourceView);
			device->CreateShaderResourceView(&gpuBuffer.GetNative(), &srvDesc.value(),
											 descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	std::optional<Descriptor> Device::CreateUnorderedAccessView(GPUBuffer& gpuBuffer)
	{
		const GPUBufferDesc&							desc = gpuBuffer.GetDesc();
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc = desc.ToUnorderedAccessViewDesc();
		if (uavDesc)
		{
			std::optional<Descriptor> descriptor =
				cbvSrvUavDescriptorHeap->Allocate(EDescriptorType::UnorderedResourceView);
			device->CreateUnorderedAccessView(&gpuBuffer.GetNative(), nullptr, &uavDesc.value(),
											  descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	std::optional<Descriptor> Device::CreateConstantBufferView(GPUBuffer& gpuBuffer)
	{
		const GPUBufferDesc& desc = gpuBuffer.GetDesc();

		std::optional<D3D12_CONSTANT_BUFFER_VIEW_DESC> cbvDesc =
			desc.ToConstantBufferViewDesc(gpuBuffer.GetNative().GetGPUVirtualAddress());
		if (cbvDesc)
		{
			std::optional<Descriptor> descriptor =
				cbvSrvUavDescriptorHeap->Allocate(EDescriptorType::ConstantBufferView);
			device->CreateConstantBufferView(&cbvDesc.value(), descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	std::optional<Descriptor> Device::CreateShaderResourceView(GPUTexture&					texture,
															   const GPUTextureSubresource& subresource)
	{
		const GPUTextureDesc&						   desc = texture.GetDesc();
		std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srvDesc = desc.ToShaderResourceViewDesc(subresource);
		if (srvDesc)
		{
			std::optional<Descriptor> descriptor =
				cbvSrvUavDescriptorHeap->Allocate(EDescriptorType::ShaderResourceView);
			device->CreateShaderResourceView(&texture.GetNative(), &srvDesc.value(),
											 descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	std::optional<Descriptor> Device::CreateUnorderedAccessView(GPUTexture&					 texture,
																const GPUTextureSubresource& subresource)
	{
		const GPUTextureDesc&							desc = texture.GetDesc();
		std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uavDesc = desc.ToUnorderedAccessViewDesc(subresource);
		if (uavDesc)
		{
			std::optional<Descriptor> descriptor =
				cbvSrvUavDescriptorHeap->Allocate(EDescriptorType::UnorderedResourceView);
			device->CreateUnorderedAccessView(&texture.GetNative(), nullptr, &uavDesc.value(),
											  descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	std::optional<Descriptor> Device::CreateRenderTargetView(GPUTexture&				  texture,
															 const GPUTextureSubresource& subresource)
	{
		const GPUTextureDesc&						 desc = texture.GetDesc();
		std::optional<D3D12_RENDER_TARGET_VIEW_DESC> rtvDesc = desc.ToRenderTargetViewDesc(subresource);
		if (rtvDesc)
		{
			std::optional<Descriptor> descriptor = rtvDescriptorHeap->Allocate(EDescriptorType::RenderTargetView);
			device->CreateRenderTargetView(&texture.GetNative(), &rtvDesc.value(),
										   descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	std::optional<Descriptor> Device::CreateDepthStencilView(GPUTexture&				  texture,
															 const GPUTextureSubresource& subresource)
	{
		const GPUTextureDesc&						 desc = texture.GetDesc();
		std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> dsvDesc = desc.ToDepthStencilViewDesc(subresource);
		if (dsvDesc)
		{
			std::optional<Descriptor> descriptor = dsvDescriptorHeap->Allocate(EDescriptorType::DepthStencilView);
			device->CreateDepthStencilView(&texture.GetNative(), &dsvDesc.value(),
										   descriptor->GetCPUDescriptorHandle());

			return descriptor;
		}

		return std::nullopt;
	}

	bool Device::AcquireAdapterFromFactory()
	{
		uint32_t factoryCreationFlags = 0;
		{
#if defined(DEBUG) || defined(_DEBUG)
			factoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
			ComPtr<ID3D12Debug5> debugController;
			const bool			 bDebugControllerAcquired = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
			FE_CONDITIONAL_LOG(D3D12Fatal, bDebugControllerAcquired, "Failed to get debug controller.");
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

		ComPtr<IDXGIFactory6> factory;
		const bool			  bFactoryCreated = SUCCEEDED(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&factory)));
		FE_CONDITIONAL_LOG(D3D12Fatal, bFactoryCreated, "Failed to create factory.");

		if (bFactoryCreated)
		{
			const bool bIsAdapterAcquired = SUCCEEDED(
				factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
			FE_CONDITIONAL_LOG(D3D12Fatal, bIsAdapterAcquired, "Failed to acquire adapter from factory.");
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
		const bool					bIsDeviceCreated =
			SUCCEEDED(D3D12CreateDevice(adapter.Get(), MinimumFeatureLevel, IID_PPV_ARGS(&device)));
		FE_CONDITIONAL_LOG(D3D12Fatal, bIsDeviceCreated, "Failed to create the device from the adapter.");
		return bIsDeviceCreated;
	}

	void Device::SetSeverityLevel()
	{
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
			FE_LOG(D3D12Warn, "Failed to query a info queue from the device.");
		}
#endif
	}

	void Device::CheckSupportedFeatures()
	{

		CD3DX12FeatureSupport features;
		features.Init(device.Get());

		if (features.EnhancedBarriersSupported())
		{
			FE_LOG(D3D12Info, "Enhanced Barriers supported.");
		}
		else
		{
			FE_LOG(D3D12Fatal, "Enhanced Barriers does not supported.");
		}

		/**
		 * Ray-tracing Tier:
		 * https://github.com/microsoft/DirectX-Specs/blob/master/d3d/Raytracing.md#d3d12_raytracing_tier
		 */
		switch (features.RaytracingTier())
		{
			case D3D12_RAYTRACING_TIER_NOT_SUPPORTED:
				FE_LOG(D3D12Info, "Raytracing does not supported.");
				break;
			case D3D12_RAYTRACING_TIER_1_0:
				FE_LOG(D3D12Info, "Raytracing Tier 1.0 supported.");
				break;
			case D3D12_RAYTRACING_TIER_1_1:
				FE_LOG(D3D12Info, "Raytracing Tier 1.1 supported.");
				break;
		}

		if (features.HighestShaderModel() >= D3D_SHADER_MODEL_6_6)
		{
			FE_LOG(D3D12Info, "Least Shader model 6.6 supported.");
		}
		else
		{
			FE_LOG(D3D12Fatal, "Shader Model 6.6 does not supported.");
		}

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

		FE_LOG(D3D12Info, "* CBV-SRV-UAV Descriptor Handle Increment Size: {}", cbvSrvUavDescriptorHandleIncrementSize);
		FE_LOG(D3D12Info, "* Sampler Descriptor Handle Increment Size: {}", samplerDescritorHandleIncrementSize);
		FE_LOG(D3D12Info, "* DSV Descriptor Handle Increment Size: {}", dsvDescriptorHandleIncrementSize);
		FE_LOG(D3D12Info, "* RTV Descriptor Handle Increment Size: {}", rtvDescriptorHandleIncrementSize);
	}

	bool Device::CreateMemoryAllcator()
	{
		D3D12MA::ALLOCATOR_DESC desc{};
		desc.pAdapter = adapter.Get();
		desc.pDevice = device.Get();
		const bool bSucceeded = SUCCEEDED(D3D12MA::CreateAllocator(&desc, &allocator));
		FE_CONDITIONAL_LOG(D3D12Fatal, bSucceeded, "Failed to create D3D12MA::Allocator.");
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

	std::optional<GPUBuffer> Device::CreateBuffer(const GPUBufferDesc& bufferDesc)
	{
		const D3D12MA::ALLOCATION_DESC allocationDesc = bufferDesc.ToAllocationDesc();
		ComPtr<D3D12MA::Allocation>	   allocation{};
		ComPtr<ID3D12Resource>		   resource{};
		if (!SUCCEEDED(allocator->CreateResource3(&allocationDesc, &bufferDesc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr,
												  0, nullptr, allocation.GetAddressOf(), IID_PPV_ARGS(&resource))))
		{
			return std::nullopt;
		}

		return GPUBuffer{ bufferDesc, std::move(allocation), std::move(resource) };
	}

	std::optional<GPUTexture> Device::CreateTexture(const GPUTextureDesc& textureDesc)
	{
		const D3D12MA::ALLOCATION_DESC allocationDesc = textureDesc.ToAllocationDesc();
		ComPtr<D3D12MA::Allocation>	   allocation{};
		ComPtr<ID3D12Resource>		   resource{};

		std::optional<D3D12_CLEAR_VALUE> clearValue{};
		if (textureDesc.IsRenderTargetCompatible())
		{
			// #todo additional for render target/depth stencil creation
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

		if (!SUCCEEDED(allocator->CreateResource3(&allocationDesc, &textureDesc,
												  D3D12_BARRIER_LAYOUT_UNDEFINED,
												  clearValue ? &clearValue.value() : nullptr,
												  0, nullptr,
												  allocation.GetAddressOf(), IID_PPV_ARGS(&resource))))
		{
			return std::nullopt;
		}

		SetObjectName(resource.Get(), textureDesc.DebugName);
		return GPUTexture{ textureDesc, std::move(allocation), std::move(resource) };
	}

	std::optional<Fence> Device::CreateFence(const std::string_view debugName, const uint64_t initialCounter /*= 0*/)
	{
		ComPtr<ID3D12Fence> newFence{};
		if (!SUCCEEDED(device->CreateFence(initialCounter, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&newFence))))
		{
			return std::nullopt;
		}

		SetObjectName(newFence.Get(), debugName);
		return Fence{ std::move(newFence) };
	}

	std::optional<PipelineState> Device::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
	{
		ComPtr<ID3D12PipelineState> newPipelineState{};
		if (!SUCCEEDED(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPipelineState))))
		{
			return std::nullopt;
		}

		// SetObjectName(newPipelineState.Get(), desc.Name);
		return PipelineState{ std::move(newPipelineState), true };
	}

	std::optional<PipelineState> Device::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
	{
		ComPtr<ID3D12PipelineState> newPipelineState{};
		if (!SUCCEEDED(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newPipelineState))))
		{
			return std::nullopt;
		}

		SetObjectName(newPipelineState.Get(), desc.Name);
		return PipelineState{ std::move(newPipelineState), false };
	}

	std::optional<RootSignature> Device::CreateBindlessRootSignature()
	{
		// WITH THOSE FLAGS, IT MUST BIND DESCRIPTOR HEAP FIRST BEFORE BINDING ROOT SIGNATURE
		// D3D12_DESCRIPTOR_RANGE			texture2DRange{};
		// texture2DRange.BaseShaderRegister = 0;
		// texture2DRange.NumDescriptors = 1024;
		// texture2DRange.OffsetInDescriptorsFromTableStart = 0;
		// texture2DRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		// texture2DRange.RegisterSpace = 0;

		// D3D12_ROOT_PARAMETER texture2DTable{};
		// texture2DTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		// texture2DTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		// texture2DTable.DescriptorTable.NumDescriptorRanges = 1;
		// texture2DTable.DescriptorTable.pDescriptorRanges = &texture2DRange;

		const D3D12_ROOT_SIGNATURE_DESC desc{ .NumParameters = 0,
											  .pParameters = nullptr,
											  .NumStaticSamplers = 0,
											  .pStaticSamplers = nullptr,
											  .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
													   D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED |
													   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };

		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;
		if (!SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0,
												   rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf())))
		{
			FE_LOG(D3D12Fatal, (const char*)errorBlob->GetBufferPointer());
			return std::nullopt;
		}

		ComPtr<ID3D12RootSignature> newRootSignature;
		if (!SUCCEEDED(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
												   rootSignatureBlob->GetBufferSize(),
												   IID_PPV_ARGS(&newRootSignature))))
		{
			return std::nullopt;
		}

		return RootSignature{ std::move(newRootSignature) };
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

	std::optional<CommandQueue> Device::CreateCommandQueue(const EQueueType queueType)
	{
		ComPtr<ID3D12CommandQueue>	  newCmdQueue;
		const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(queueType);
		check(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

		const D3D12_COMMAND_QUEUE_DESC desc = { .Type = cmdListType,
												.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
												.NodeMask = 0 };

		if (!SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&newCmdQueue))))
		{
			return std::nullopt;
		}

		return CommandQueue{ std::move(newCmdQueue), queueType };
	}

	std::optional<CommandContext> Device::CreateCommandContext(const EQueueType targetQueueType)
	{
		ComPtr<ID3D12CommandAllocator>	   newCmdAllocator;
		ComPtr<ID3D12GraphicsCommandList7> newCmdList;

		const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(targetQueueType);
		check(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

		const D3D12_COMMAND_LIST_FLAGS flags = D3D12_COMMAND_LIST_FLAG_NONE;

		// #todo 스레드 당 Command Allocator 할당
		if (!SUCCEEDED(device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&newCmdAllocator))))
		{
			return {};
		}
		if (!SUCCEEDED(device->CreateCommandList1(0, cmdListType, flags, IID_PPV_ARGS(&newCmdList))))
		{
			return {};
		}

		return CommandContext{ std::move(newCmdAllocator), std::move(newCmdList), targetQueueType };
	}

	std::optional<DescriptorHeap> Device::CreateDescriptorHeap(const EDescriptorHeapType descriptorHeapType,
															   const uint32_t			 numDescriptors)
	{
		D3D12_DESCRIPTOR_HEAP_TYPE targetDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
		switch (descriptorHeapType)
		{
			case EDescriptorHeapType::CBV_SRV_UAV:
				targetDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				break;
			case EDescriptorHeapType::Sampler:
				targetDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				break;
			case EDescriptorHeapType::DSV:
				targetDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				break;
			case EDescriptorHeapType::RTV:
				targetDescriptorHeapType = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				break;
		}
		check(targetDescriptorHeapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
		const bool bIsShaderVisibleDescriptorHeap = descriptorHeapType == EDescriptorHeapType::CBV_SRV_UAV ||
													descriptorHeapType == EDescriptorHeapType::Sampler;

		const D3D12_DESCRIPTOR_HEAP_DESC desc{ .Type = targetDescriptorHeapType,
											   .NumDescriptors = numDescriptors,
											   .Flags = bIsShaderVisibleDescriptorHeap
															? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
															: D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
											   .NodeMask = 0 };

		ComPtr<ID3D12DescriptorHeap> newDescriptorHeap;
		if (!SUCCEEDED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeap))))
		{
			return std::nullopt;
		}

		return DescriptorHeap{ descriptorHeapType, std::move(newDescriptorHeap), bIsShaderVisibleDescriptorHeap,
							   numDescriptors, GetDescriptorHandleIncrementSize(descriptorHeapType) };
	}

	std::array<std::reference_wrapper<DescriptorHeap>, 2> Device::GetBindlessDescriptorHeaps()
	{
		return { *cbvSrvUavDescriptorHeap, *samplerDescriptorHeap };
	}

} // namespace fe::dx