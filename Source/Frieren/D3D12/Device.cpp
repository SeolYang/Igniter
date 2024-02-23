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
				}
			}
		}
	}

	Device::~Device()
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

		// #todo #wip 각 타입에 맞는 Bindless Descriptor Range 설정 및 생성
		// WITH THOSE FLAGS, IT MUST BIND DESCRIPTOR HEAP FIRST BEFORE BINDING ROOT SIGNATURE
		// ------- 참고자료 확인 전 아이디어 --------
		// Root Signature 를 Shader Reflection 으로 생성 하는 방법도 알아낼필요가 있을 것 같음.
		// device.CreateRootSignatureFromShader(shader) -> Reflecting shader -> create rootsignature desc -> hash roogsignature desc -> if DNE in table -> create new root signature object
		// 쉐이더에서 bindless descriptor 를 활용하기 위해선
		// 1. 각 View 타입에 따라 register space 할당
		// 2. 쉐이더에서, 접근할 View 의 Index 를 알아야함.  (GPUView.Index -> Constant Buffer -> Use in shader to access specific descriptor )
		// 3. (2)를 위해서, 특별한 Constant Buffer 가 필요함. View 의 Index 는 생성될 때 확정, 얘네를 아에 inline descriptor 로 보내버리거나.. root constant로 공급 해주는
		// 방법도 있긴함.
		// 쉐이더 레지스터 스페이스등에 대한 자세한 이해가 필요
		// -----------------------------------------
		// 참고자료 =>
		// https://rtarun9.github.io/blogs/bindless_rendering/
		// Raytacing Gems 2 - Chapter 17
		// ------- 참고자료 확인 후 아이디어 -------
		//  Root Signature 를 통해 총 64 DWORD 분량의 데이터를 제공 할 수 있다.
		//  Root Parameter(Root Constant, Root Descriptor, Root Descriptor Table) 중에서
		//  Root Constant = 1DWORD (4바이트) 크기이며, Set(Graphics/Compute)Root32BitsConstant 를 통해, 총 64*4 바이트의
		//  데이터를, 한 개의 루트 시그니처의 argument 로 포함시켜서 쉐이더에 제공 할 수 있다.
		//  즉, 이를 통해 descriptor(view)의 index 를 쉐이더에 제공 할 수 있다.
		// 추가 참고자료 => https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
		// 레이트레이싱 젬의 내용은 다소 이전 단계의 Shader Model 를 위한 것으로 보임.
		// 현재 프로젝트의 목표가 SM 6.6+를 타겟팅 하고있는 만큼. 이는 배제할 것.
		// 내부 index 는 uint32_t로, 즉 32 bits = 1 DWORD = 4 바이트
		// 위의 Root Constant 로도 총 64개의 뷰 인덱스를 보낼 수 있고.
		// 더 많은 인덱스가 필요하더라도, 추가적인 structured buffer/constant buffer 등을 통해 제공 가능
		// 인덱스의 Uniform에 대하여
		// https://www.asawicki.info/news_1608_direct3d_12_-_watch_out_for_non-uniform_resource_index
		// https://www.asawicki.info/news_1734_which_values_are_scalar_in_a_shader
		// Bindless Descriptor Heap은 volatile이여야 한다. 즉 GPU Executing 이전에는 descriptor 가 가르키는 데이터/descriptor 자체에
		// 대한 CPU의 Read-Write가 하용된다는 것이다. (즉 root signature version != 1.1)
		// 쉐이더와 매치되는 Root Signature를 어떻게 만들지?
		// -----------------------------------------

		constexpr uint8_t		   NumReservedConstants = 16;
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
			FE_LOG(DeviceFatal, "Failed to create buffer. HRESULT: {}", result);
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

	ComPtr<D3D12MA::Pool> Device::CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc)
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

	void Device::UpdateConstantBufferView(const GPUView& gpuView, GPUBuffer& buffer)
	{
		check(gpuView.Type == EGPUViewType::ConstantBufferView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GPUBufferDesc&						   desc = buffer.GetDesc();
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

	void Device::UpdateConstantBufferView(const GPUView& gpuView, GPUBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes)
	{
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GPUBufferDesc& desc = buffer.GetDesc();
		check((offset + sizeInBytes) < desc.GetSizeAsBytes());
		if (gpuView.Type == EGPUViewType::ConstantBufferView)
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

	void Device::UpdateShaderResourceView(const GPUView& gpuView, GPUBuffer& buffer)
	{
		check(gpuView.Type == EGPUViewType::ShaderResourceView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GPUBufferDesc&						   desc = buffer.GetDesc();
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

	void Device::UpdateUnorderedAccessView(const GPUView& gpuView, GPUBuffer& buffer)
	{
		check(gpuView.Type == EGPUViewType::UnorderedAccessView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(buffer);
		const GPUBufferDesc&							desc = buffer.GetDesc();
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

	void Device::UpdateShaderResourceView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource)
	{
		check(gpuView.Type == EGPUViewType::ShaderResourceView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc&						   desc = texture.GetDesc();
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

	void Device::UpdateUnorderedAccessView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource)
	{
		check(gpuView.Type == EGPUViewType::UnorderedAccessView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc&							desc = texture.GetDesc();
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

	void Device::UpdateRenderTargetView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource)
	{
		check(gpuView.Type == EGPUViewType::RenderTargetView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc&						 desc = texture.GetDesc();
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

	void Device::UpdateDepthStencilView(const GPUView& gpuView, GPUTexture& texture, const GPUTextureSubresource& subresource)
	{
		check(gpuView.Type == EGPUViewType::DepthStencilView);
		check(gpuView.IsValid() && gpuView.HasValidCPUHandle());
		check(texture);
		const GPUTextureDesc&						 desc = texture.GetDesc();
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

} // namespace fe::dx