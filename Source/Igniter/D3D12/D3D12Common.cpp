#include "Igniter/Igniter.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/Core/String.h"

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
}

extern "C"
{
    __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

namespace ig
{
    D3D12_COMMAND_LIST_TYPE ToNativeCommandListType(const EQueueType type)
    {
        switch (type)
        {
        case EQueueType::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case EQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case EQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        }

        return D3D12_COMMAND_LIST_TYPE_NONE;
    }

    bool IsSupportGpuView(const EDescriptorHeapType descriptorHeapType, const EGpuViewType gpuViewType)
    {
        switch (descriptorHeapType)
        {
        case EDescriptorHeapType::CBV_SRV_UAV:
            return gpuViewType == EGpuViewType::ShaderResourceView || gpuViewType == EGpuViewType::ConstantBufferView ||
                gpuViewType == EGpuViewType::UnorderedAccessView;
        case EDescriptorHeapType::Sampler:
            return gpuViewType == EGpuViewType::Sampler;
        case EDescriptorHeapType::RTV:
            return gpuViewType == EGpuViewType::RenderTargetView;
        case EDescriptorHeapType::DSV:
            return gpuViewType == EGpuViewType::DepthStencilView;
        }

        return false;
    }

    void SetObjectName(ID3D12Object* object, const std::string_view name)
    {
        if (object != nullptr)
        {
            object->SetName(Utf8ToUtf16(name).c_str());
        }
    }
} // namespace ig
