#include "Igniter/Igniter.h"
#include "Igniter/Core/Log.h"
#include "Igniter/D3D12/GpuDevice.h"
#include "Igniter/D3D12/CommandQueue.h"
#include "Igniter/D3D12/CommandList.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/D3D12/GPUView.h"
#include "Igniter/D3D12/GPUBufferDesc.h"
#include "Igniter/D3D12/GPUBuffer.h"
#include "Igniter/D3D12/GPUTextureDesc.h"
#include "Igniter/D3D12/GPUTexture.h"
#include "Igniter/D3D12/GpuFence.h"
#include "Igniter/D3D12/CommandSignature.h"
#include "Igniter/D3D12/CommandSignatureDesc.h"

IG_DECLARE_LOG_CATEGORY(GpuDeviceLog);

IG_DEFINE_LOG_CATEGORY(GpuDeviceLog);

namespace ig
{
    GpuDevice::GpuDevice()
    {
        if (!AcquireAdapterFromFactory())
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to acquire adapter from factory.");
            return;
        }
        AcquireAdapterInfo();

        if (!CreateDevice())
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create D3D12 device.");
            return;
        }
        SetObjectName(device.Get(), "Device");
        SetSeverityLevel();
        CheckSupportedFeatures();

        if (!CreateMemoryAllocator())
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create D3D12 memory allocator.");
        }
        CacheDescriptorHandleIncrementSize();
    }

    GpuDevice::~GpuDevice()
    {
        allocator->Release();
        device.Reset();
        adapter.Reset();

#ifdef _DEBUG
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            [[maybe_unused]] const HRESULT result =
                dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
#endif
    }

    U32 GpuDevice::GetDescriptorHandleIncrementSize(const EDescriptorHeapType type) const
    {
        switch (type)
        {
        default:
        case EDescriptorHeapType::CBV_SRV_UAV:
            return cbvSrvUavDescriptorHandleIncrementSize;
        case EDescriptorHeapType::Sampler:
            return samplerDescriptorHandleIncrementSize;
        case EDescriptorHeapType::DSV:
            return dsvDescriptorHandleIncrementSize;
        case EDescriptorHeapType::RTV:
            return rtvDescriptorHandleIncrementSize;
        }
    }

    bool GpuDevice::AcquireAdapterFromFactory()
    {
        U32 factoryCreationFlags = 0;
        {
#if defined(DEBUG) || defined(_DEBUG)
            factoryCreationFlags |= DXGI_CREATE_FACTORY_DEBUG;
            ComPtr<ID3D12Debug5> debugController;
            const bool bDebugControllerAcquired = SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
            if (!bDebugControllerAcquired)
            {
                IG_LOG(GpuDeviceLog, Warning, "Failed to get debug controller.");
            }
            if (bDebugControllerAcquired)
            {
                debugController->EnableDebugLayer();
                IG_LOG(GpuDeviceLog, Info, "D3D12 Debug Layer Enabled.");
                debugController->SetEnableGPUBasedValidation(true);
                IG_LOG(GpuDeviceLog, Info, "D3D12 GPU Based Validation Enabled.");
            }
#endif
        }

        ComPtr<IDXGIFactory6> factory;
        const bool bFactoryCreated = SUCCEEDED(CreateDXGIFactory2(factoryCreationFlags, IID_PPV_ARGS(&factory)));
        if (!bFactoryCreated)
        {
            IG_LOG(GpuDeviceLog, Fatal, "Failed to create factory.");
            return false;
        }

        const bool bIsAdapterAcquired =
            SUCCEEDED(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
        if (!bIsAdapterAcquired)
        {
            IG_LOG(GpuDeviceLog, Fatal, "Failed to acquire adapter from factory.");
            return false;
        }

        return true;
    }

    void GpuDevice::AcquireAdapterInfo()
    {
        IG_CHECK(adapter);
        DXGI_ADAPTER_DESC adapterDesc;
        adapter->GetDesc(&adapterDesc);

        name = Utf16ToUtf8(adapterDesc.Description);
        IG_LOG(GpuDeviceLog, Info, "----------- The GPU Infos -----------");
        IG_LOG(GpuDeviceLog, Info, "{}", name);
        IG_LOG(GpuDeviceLog, Info, "Vendor ID: {}", adapterDesc.VendorId);
        IG_LOG(GpuDeviceLog, Info, "Dedicated Video Memory: {}", BytesToMegaBytes(adapterDesc.DedicatedVideoMemory));
        IG_LOG(GpuDeviceLog, Info, "Dedicated System Memory: {}", BytesToMegaBytes(adapterDesc.DedicatedSystemMemory));
        IG_LOG(GpuDeviceLog, Info, "Shared System Memory: {}", BytesToMegaBytes(adapterDesc.SharedSystemMemory));
        IG_LOG(GpuDeviceLog, Info, "-------------------------------------");
    }

    bool GpuDevice::CreateDevice()
    {
        IG_CHECK(adapter);
        constexpr D3D_FEATURE_LEVEL MinimumFeatureLevel = D3D_FEATURE_LEVEL_12_2;
        if (!SUCCEEDED(D3D12CreateDevice(adapter.Get(), MinimumFeatureLevel, IID_PPV_ARGS(&device))))
        {
            IG_LOG(GpuDeviceLog, Fatal, "Failed to create the device from the adapter.");
            return false;
        }

        IG_CHECK(device);
        return true;
    }

    void GpuDevice::SetSeverityLevel()
    {
        IG_CHECK(device);
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
            IG_LOG(GpuDeviceLog, Warning, "Failed to query a info queue from the device.");
        }
#endif
    }

    void GpuDevice::CheckSupportedFeatures()
    {
        IG_CHECK(device);
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
        default:
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

        /*
         * #sy_todo Virtual Texturing using tiled resource
         * D3D12_TILED_RESOURCES_TIER > 2 for virtual texturing + texture streaming
         * https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_tiled_resources_tier
         */
    }

    void GpuDevice::CacheDescriptorHandleIncrementSize()
    {
        cbvSrvUavDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        samplerDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        dsvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        rtvDescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    bool GpuDevice::CreateMemoryAllocator()
    {
        IG_CHECK(device);
        D3D12MA::ALLOCATOR_DESC desc{};
        desc.pAdapter = adapter.Get();
        desc.pDevice = device.Get();
        if (!SUCCEEDED(D3D12MA::CreateAllocator(&desc, &allocator)))
        {
            IG_LOG(GpuDeviceLog, Fatal, "Failed to create D3D12MA::Allocator.");
            return false;
        }

        IG_CHECK(allocator);
        return true;
    }

    GpuDevice::Statistics GpuDevice::GetStatistics() const
    {
        IG_CHECK(device);
        IG_CHECK(allocator != nullptr);

        Statistics statistics{};
        statistics.DeviceName = name;
        statistics.bIsUma = allocator->IsUMA();

        D3D12MA::Budget localBudget{};
        D3D12MA::Budget nonLocalBudget{};
        allocator->GetBudget(&localBudget, &nonLocalBudget);
        if (!statistics.bIsUma)
        {
            statistics.DedicatedVideoMemUsage = localBudget.UsageBytes;
            statistics.DedicatedVideoMemBudget = localBudget.BudgetBytes;
            statistics.DedicatedVideoMemBlockCount = localBudget.Stats.BlockCount;
            statistics.DedicatedVideoMemBlockSize = localBudget.Stats.BlockBytes;
            statistics.DedicatedVideoMemAllocCount = localBudget.Stats.AllocationCount;
            statistics.DedicatedVideoMemAllocSize = localBudget.Stats.AllocationBytes;
            statistics.SharedVideoMemUsage = nonLocalBudget.UsageBytes;
            statistics.SharedVideoMemBudget = nonLocalBudget.BudgetBytes;
            statistics.SharedVideoMemBlockCount = nonLocalBudget.Stats.BlockCount;
            statistics.SharedVideoMemBlockSize = nonLocalBudget.Stats.BlockBytes;
            statistics.SharedVideoMemAllocCount = nonLocalBudget.Stats.AllocationCount;
            statistics.SharedVideoMemAllocSize = nonLocalBudget.Stats.AllocationBytes;
        }
        else
        {
            statistics.SharedVideoMemUsage = nonLocalBudget.UsageBytes;
            statistics.SharedVideoMemBudget = nonLocalBudget.BudgetBytes;
            statistics.SharedVideoMemBlockCount = nonLocalBudget.Stats.BlockCount;
            statistics.SharedVideoMemBlockSize = nonLocalBudget.Stats.BlockBytes;
            statistics.SharedVideoMemAllocCount = nonLocalBudget.Stats.AllocationCount;
            statistics.SharedVideoMemAllocSize = nonLocalBudget.Stats.AllocationBytes;
        }

        return statistics;
    }

    Option<CommandQueue> GpuDevice::CreateCommandQueue(const std::string_view debugName, const EQueueType queueType)
    {
        IG_CHECK(device);

        const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(queueType);
        IG_CHECK(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

        const D3D12_COMMAND_QUEUE_DESC desc = {.Type = cmdListType, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE, .NodeMask = 0};

        ComPtr<ID3D12CommandQueue> newCmdQueue;
        if (const HRESULT result = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&newCmdQueue)); !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create command queue. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newCmdQueue);
        SetObjectName(newCmdQueue.Get(), debugName);
        return CommandQueue{std::move(newCmdQueue), CreateFence(std::format("{}.Fence", debugName)).value(), queueType};
    }

    Option<CommandList> GpuDevice::CreateCommandList(const std::string_view debugName, const EQueueType targetQueueType)
    {
        IG_CHECK(device);

        const D3D12_COMMAND_LIST_TYPE cmdListType = ToNativeCommandListType(targetQueueType);
        IG_CHECK(cmdListType != D3D12_COMMAND_LIST_TYPE_NONE);

        ComPtr<ID3D12CommandAllocator> newCmdAllocator;
        if (const HRESULT result = device->CreateCommandAllocator(cmdListType, IID_PPV_ARGS(&newCmdAllocator)); !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create command allocator. HRESULT: {:#X}", result);
            return {};
        }

        ComPtr<ID3D12GraphicsCommandList7> newCmdList;
        const D3D12_COMMAND_LIST_FLAGS flags = D3D12_COMMAND_LIST_FLAG_NONE;
        if (const HRESULT result = device->CreateCommandList1(0, cmdListType, flags, IID_PPV_ARGS(&newCmdList)); !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create command list. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newCmdAllocator);
        IG_CHECK(newCmdList);
        SetObjectName(newCmdList.Get(), debugName);
        return CommandList{std::move(newCmdAllocator), std::move(newCmdList), targetQueueType};
    }

    Option<RootSignature> GpuDevice::CreateBindlessRootSignature()
    {
        IG_CHECK(device);
        constexpr uint8_t NumReservedConstants = 16; // 16 DWORDS / 64 DWORDS
        const D3D12_ROOT_PARAMETER rootParam{
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = {.ShaderRegister = 0, .RegisterSpace = 0, .Num32BitValues = NumReservedConstants},
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        };

        const D3D12_ROOT_SIGNATURE_DESC desc{
            .NumParameters = 1,
            .pParameters = &rootParam,
            .NumStaticSamplers = 0,
            .pStaticSamplers = nullptr,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
        };

        ComPtr<ID3DBlob> errorBlob;
        ComPtr<ID3DBlob> rootSignatureBlob;
        if (const HRESULT result =
                D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf());
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to serialize root signature. HRESULT: {:#X}, Message: {}", result, errorBlob->GetBufferPointer());
            return {};
        }

        ComPtr<ID3D12RootSignature> newRootSignature;
        if (const HRESULT result = device->CreateRootSignature(
                0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&newRootSignature));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create root signature. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newRootSignature);
        return RootSignature{std::move(newRootSignature)};
    }

    Option<PipelineState> GpuDevice::CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc)
    {
        IG_CHECK(device);
        IG_CHECK(desc.pRootSignature != nullptr);
        IG_CHECK(desc.VS.pShaderBytecode != nullptr && desc.VS.BytecodeLength > 0);
        IG_CHECK(desc.PS.pShaderBytecode != nullptr && desc.PS.BytecodeLength > 0);

        ComPtr<ID3D12PipelineState> newPipelineState{};

        if (const HRESULT result = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&newPipelineState));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create graphics pipeline state. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newPipelineState);
        SetObjectName(newPipelineState.Get(), desc.Name);
        return PipelineState{std::move(newPipelineState), true};
    }

    Option<PipelineState> GpuDevice::CreateComputePipelineState(const ComputePipelineStateDesc& desc)
    {
        IG_CHECK(device);
        IG_CHECK(desc.pRootSignature != nullptr);
        IG_CHECK(desc.CS.pShaderBytecode != nullptr && desc.CS.BytecodeLength > 0);

        ComPtr<ID3D12PipelineState> newPipelineState{};
        if (const HRESULT result = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&newPipelineState));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create compute pipeline state. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newPipelineState);
        SetObjectName(newPipelineState.Get(), desc.Name);
        return PipelineState{std::move(newPipelineState), false};
    }

    Option<PipelineState> GpuDevice::CreateMeshShaderPipelineState(const MeshShaderPipelineStateDesc& desc)
    {
        IG_CHECK(device);
        IG_CHECK(desc.pRootSignature != nullptr);
        IG_CHECK(desc.AS.pShaderBytecode != nullptr && desc.AS.BytecodeLength > 0);
        IG_CHECK(desc.MS.pShaderBytecode != nullptr && desc.MS.BytecodeLength > 0);

        auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(desc);
        const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc{
            .SizeInBytes = sizeof(psoStream),
            .pPipelineStateSubobjectStream = &psoStream
        };

        ComPtr<ID3D12PipelineState> newPipelineState{};
        if (const HRESULT result = device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&newPipelineState));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create mesh shader pipeline state. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newPipelineState);
        SetObjectName(newPipelineState.Get(), desc.Name);
        return PipelineState{std::move(newPipelineState), true};
    }

    Option<DescriptorHeap> GpuDevice::CreateDescriptorHeap(const std::string_view debugName, const EDescriptorHeapType descriptorHeapType, const U32 numDescriptors)
    {
        IG_CHECK(device);

        const D3D12_DESCRIPTOR_HEAP_TYPE targetDescriptorHeapType = ToNativeDescriptorHeapType(descriptorHeapType);
        IG_CHECK(targetDescriptorHeapType != D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);

        const bool bIsShaderVisibleDescriptorHeap = IsShaderVisibleDescriptorHeapType(descriptorHeapType);

        const D3D12_DESCRIPTOR_HEAP_DESC desc{
            .Type = targetDescriptorHeapType,
            .NumDescriptors = numDescriptors,
            .Flags = bIsShaderVisibleDescriptorHeap ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask = 0
        };

        ComPtr<ID3D12DescriptorHeap> newDescriptorHeap;
        if (const HRESULT result = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&newDescriptorHeap)); !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create descriptor heap. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newDescriptorHeap);
        SetObjectName(newDescriptorHeap.Get(), debugName);
        return DescriptorHeap{
            descriptorHeapType, std::move(newDescriptorHeap), bIsShaderVisibleDescriptorHeap, numDescriptors,
            GetDescriptorHandleIncrementSize(descriptorHeapType)
        };
    }

    Option<GpuFence> GpuDevice::CreateFence(const std::string_view debugName)
    {
        ComPtr<ID3D12Fence> newFence{};
        if (const HRESULT result = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&newFence)); !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create queue sync fence. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(newFence);
        SetObjectName(newFence.Get(), debugName);
        return GpuFence{std::move(newFence)};
    }

    Option<CommandSignature> GpuDevice::CreateCommandSignature(const std::string_view debugName, const CommandSignatureDesc& desc, const Option<Ref<RootSignature>> rootSignatureOpt)
    {
        ComPtr<ID3D12CommandSignature> newCommandSignature{};

        const Option<D3D12_COMMAND_SIGNATURE_DESC> cmdSignatureDescOpt = desc.ToCommandSignatureDesc();
        if (!cmdSignatureDescOpt.has_value())
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create Command Signature Desc.");
            return None<CommandSignature>();
        }

        ID3D12RootSignature* const nativeRootSignature = rootSignatureOpt ? &rootSignatureOpt->get().GetNative() : nullptr;
        if (const HRESULT result = device->CreateCommandSignature(&cmdSignatureDescOpt.value(), nativeRootSignature, IID_PPV_ARGS(&newCommandSignature));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create Command Signature. HRESULT: {:#X}", result);
            return None<CommandSignature>();
        }

        IG_CHECK(newCommandSignature);
        SetObjectName(newCommandSignature.Get(), debugName);
        return CommandSignature{std::move(newCommandSignature)};
    }

    Option<GpuBuffer> GpuDevice::CreateBuffer(const GpuBufferDesc& bufferDesc)
    {
        IG_CHECK(device);
        IG_CHECK(allocator);

        const D3D12MA::ALLOCATION_DESC allocationDesc = bufferDesc.GetAllocationDesc();
        ComPtr<D3D12MA::Allocation> allocation{};
        ComPtr<ID3D12Resource> resource{};
        if (const HRESULT result = allocator->CreateResource3(&allocationDesc, &bufferDesc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, 0, nullptr,
                allocation.GetAddressOf(), IID_PPV_ARGS(&resource));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create buffer resource. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(allocation);
        IG_CHECK(resource);
        SetObjectName(resource.Get(), bufferDesc.DebugName);
        return GpuBuffer{bufferDesc, std::move(allocation), std::move(resource)};
    }

    Option<GpuTexture> GpuDevice::CreateTexture(const GpuTextureDesc& textureDesc)
    {
        IG_CHECK(device);
        IG_CHECK(allocator);

        const D3D12MA::ALLOCATION_DESC allocationDesc = textureDesc.GetAllocationDesc();
        Option<D3D12_CLEAR_VALUE> clearValue{};
        if (textureDesc.IsRenderTargetCompatible())
        {
            clearValue = D3D12_CLEAR_VALUE{.Format = textureDesc.Format};
            clearValue->Color[0] = textureDesc.ClearColorValue.R();
            clearValue->Color[1] = textureDesc.ClearColorValue.G();
            clearValue->Color[2] = textureDesc.ClearColorValue.B();
            clearValue->Color[3] = textureDesc.ClearColorValue.A();
        }
        else if (textureDesc.IsDepthStencilCompatible())
        {
            clearValue = D3D12_CLEAR_VALUE{.Format = textureDesc.Format, .DepthStencil = {.Depth = textureDesc.ClearDepthValue, .Stencil = textureDesc.ClearStencilValue}};
        }

        ComPtr<D3D12MA::Allocation> allocation{};
        ComPtr<ID3D12Resource> resource{};
        IG_CHECK(textureDesc.InitialLayout != D3D12_BARRIER_LAYOUT_UNDEFINED);
        if (const HRESULT result = allocator->CreateResource3(&allocationDesc, &textureDesc, textureDesc.InitialLayout,
                clearValue ? &clearValue.value() : nullptr, 0, nullptr, allocation.GetAddressOf(), IID_PPV_ARGS(&resource));
            !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create texture resource. HRESULT: {:#X}", result);
            return {};
        }

        IG_CHECK(allocation);
        IG_CHECK(resource);
        SetObjectName(resource.Get(), textureDesc.DebugName);
        return GpuTexture{textureDesc, std::move(allocation), std::move(resource)};
    }

    ComPtr<D3D12MA::Pool> GpuDevice::CreateCustomMemoryPool(const D3D12MA::POOL_DESC& desc)
    {
        IG_CHECK(device);
        IG_CHECK(allocator != nullptr);

        ComPtr<D3D12MA::Pool> customPool;
        if (const HRESULT result = allocator->CreatePool(&desc, &customPool); !SUCCEEDED(result))
        {
            IG_LOG(GpuDeviceLog, Error, "Failed to create custom gpu memory pool.");
            return {};
        }

        return customPool;
    }

    GpuCopyableFootprints GpuDevice::GetCopyableFootprints(const D3D12_RESOURCE_DESC1& resDesc, const U32 firstSubresource, const U32 numSubresources, const uint64_t baseOffset) const
    {
        GpuCopyableFootprints footPrints{};
        footPrints.Layouts.resize(numSubresources);
        footPrints.NumRows.resize(numSubresources);
        footPrints.RowSizesInBytes.resize(numSubresources);

        device->GetCopyableFootprints1(
            &resDesc,
            firstSubresource, numSubresources,
            baseOffset,
            footPrints.Layouts.data(), footPrints.NumRows.data(), footPrints.RowSizesInBytes.data(),
            &footPrints.RequiredSize);

        IG_CHECK(footPrints.RequiredSize > 0);
        IG_CHECK(!footPrints.Layouts.empty());
        IG_CHECK(!footPrints.NumRows.empty());
        IG_CHECK(!footPrints.RowSizesInBytes.empty());
        return footPrints;
    }

    void GpuDevice::CreateSampler(const D3D12_SAMPLER_DESC& samplerDesc, const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::Sampler);
        device->CreateSampler(&samplerDesc, gpuView.CpuHandle);
    }

    void GpuDevice::DestroySampler(const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::Sampler);
        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        device->CreateSampler(&samplerDesc, gpuView.CpuHandle);
    }

    void GpuDevice::CreateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::ConstantBufferView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(buffer);
        const GpuBufferDesc& desc = buffer.GetDesc();
        const Option<D3D12_CONSTANT_BUFFER_VIEW_DESC> cbvDesc = desc.ToConstantBufferViewDesc(buffer.GetNative().GetGPUVirtualAddress());
        IG_CHECK(cbvDesc);

        device->CreateConstantBufferView(&cbvDesc.value(), gpuView.CpuHandle);
    }

    void GpuDevice::CreateConstantBufferView(const GpuView& gpuView, GpuBuffer& buffer, const uint64_t offset, const uint64_t sizeInBytes)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::ConstantBufferView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(buffer);
        IG_CHECK((offset + sizeInBytes) < buffer.GetDesc().GetSizeAsBytes());

        const D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
            .BufferLocation = buffer.GetNative().GetGPUVirtualAddress() + offset, .SizeInBytes = static_cast<U32>(sizeInBytes)
        };

        device->CreateConstantBufferView(&cbvDesc, gpuView.CpuHandle);
    }

    void GpuDevice::CreateShaderResourceView(const GpuView& gpuView, GpuBuffer& buffer, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::ShaderResourceView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(buffer);
        device->CreateShaderResourceView(&buffer.GetNative(), &srvDesc, gpuView.CpuHandle);
    }

    void GpuDevice::CreateUnorderedAccessView(const GpuView& gpuView, GpuBuffer& buffer, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::UnorderedAccessView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(buffer);
        const GpuBufferDesc& desc = buffer.GetDesc();
        IG_CHECK(!desc.IsUavCounterEnabled() || uavDesc.Buffer.CounterOffsetInBytes != 0);
        ID3D12Resource& nativeBuffer = buffer.GetNative();
        device->CreateUnorderedAccessView(
            &nativeBuffer,
            desc.IsUavCounterEnabled() ? &nativeBuffer : nullptr,
            &uavDesc,
            gpuView.CpuHandle);
    }

    void GpuDevice::CreateShaderResourceView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureSrvVariant& srvVariant, const DXGI_FORMAT desireViewFormat)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::ShaderResourceView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(texture);
        const GpuTextureDesc& desc = texture.GetDesc();
        const Option<D3D12_SHADER_RESOURCE_VIEW_DESC> nativeDesc = desc.ConvertToNativeDesc(srvVariant, desireViewFormat);
        IG_CHECK(nativeDesc);

        device->CreateShaderResourceView(&texture.GetNative(), &nativeDesc.value(), gpuView.CpuHandle);
    }

    void GpuDevice::CreateUnorderedAccessView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureUavVariant& uavVariant, const DXGI_FORMAT desireViewFormat)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::UnorderedAccessView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(texture);
        const GpuTextureDesc& desc = texture.GetDesc();
        const Option<D3D12_UNORDERED_ACCESS_VIEW_DESC> nativeDesc = desc.ConvertToNativeDesc(uavVariant, desireViewFormat);
        IG_CHECK(nativeDesc);

        device->CreateUnorderedAccessView(&texture.GetNative(), nullptr, &nativeDesc.value(), gpuView.CpuHandle);
    }

    void GpuDevice::CreateRenderTargetView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureRtvVariant& rtvVariant, const DXGI_FORMAT desireViewFormat)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::RenderTargetView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(texture);
        const GpuTextureDesc& desc = texture.GetDesc();
        const Option<D3D12_RENDER_TARGET_VIEW_DESC> nativeDesc = desc.ConvertToNativeDesc(rtvVariant, desireViewFormat);
        IG_CHECK(nativeDesc);

        device->CreateRenderTargetView(&texture.GetNative(), &nativeDesc.value(), gpuView.CpuHandle);
    }

    void GpuDevice::CreateDepthStencilView(const GpuView& gpuView, GpuTexture& texture, const GpuTextureDsvVariant& dsvVariant, const DXGI_FORMAT desireViewFormat)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.Type == EGpuViewType::DepthStencilView);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(texture);
        const GpuTextureDesc& desc = texture.GetDesc();
        const Option<D3D12_DEPTH_STENCIL_VIEW_DESC> nativeDesc = desc.ConvertToNativeDesc(dsvVariant, desireViewFormat);
        IG_CHECK(nativeDesc);

        device->CreateDepthStencilView(&texture.GetNative(), &nativeDesc.value(), gpuView.CpuHandle);
    }

    void GpuDevice::DestroyConstantBufferView(const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::ConstantBufferView);
        constexpr D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {.BufferLocation = 0, .SizeInBytes = 256};
        device->CreateConstantBufferView(&cbvDesc, gpuView.CpuHandle);
    }

    void GpuDevice::DestroyShaderResourceView(const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::ShaderResourceView);
        constexpr D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {.MipLevels = 1}
        };
        device->CreateShaderResourceView(nullptr, &srvDesc, gpuView.CpuHandle);
    }

    void GpuDevice::DestroyUnorderedAccessView(const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::UnorderedAccessView);
        constexpr D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {.Format = DXGI_FORMAT_R8G8B8A8_UNORM, .ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D};
        device->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, gpuView.CpuHandle);
    }

    void GpuDevice::DestroyRenderTargetView(const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::RenderTargetView);
        constexpr D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {.Format = DXGI_FORMAT_R8G8B8A8_UNORM, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D};
        device->CreateRenderTargetView(nullptr, &rtvDesc, gpuView.CpuHandle);
    }

    void GpuDevice::DestroyDepthStencilView(const GpuView& gpuView)
    {
        IG_CHECK(device);
        IG_CHECK(gpuView.IsValid() && gpuView.HasValidCpuHandle());
        IG_CHECK(gpuView.Type == EGpuViewType::DepthStencilView);
        constexpr D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {.Format = DXGI_FORMAT_D32_FLOAT, .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D};
        device->CreateDepthStencilView(nullptr, &dsvDesc, gpuView.CpuHandle);
    }
} // namespace ig
