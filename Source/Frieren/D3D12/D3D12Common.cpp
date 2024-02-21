#include <D3D12/Common.h>
#include <Core/String.h>

extern "C"
{
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
}
extern "C"
{
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";
}

namespace fe::dx
{
	D3D12_COMMAND_LIST_TYPE ToNativeCommandListType(const EQueueType type)
	{
		switch (type)
		{
			case EQueueType::Direct:
				return D3D12_COMMAND_LIST_TYPE_DIRECT;
			case EQueueType::AsyncCompute:
				return D3D12_COMMAND_LIST_TYPE_COMPUTE;
			case EQueueType::Copy:
				return D3D12_COMMAND_LIST_TYPE_COPY;
		}

		return D3D12_COMMAND_LIST_TYPE_NONE;
	}

	bool IsSupportGPUView(const EDescriptorHeapType descriptorHeapType, const EGPUViewType gpuViewType)
	{
		switch (descriptorHeapType)
		{
			case EDescriptorHeapType::CBV_SRV_UAV:
				return gpuViewType == EGPUViewType::ShaderResourceView ||
					   gpuViewType == EGPUViewType::ConstantBufferView ||
					   gpuViewType == EGPUViewType::UnorderedAccessView;
			case EDescriptorHeapType::Sampler:
				return gpuViewType == EGPUViewType::Sampler;
			case EDescriptorHeapType::RTV:
				return gpuViewType == EGPUViewType::RenderTargetView;
			case EDescriptorHeapType::DSV:
				return gpuViewType == EGPUViewType::DepthStencilView;
		}

		return false;
	}

	void SetObjectName(ID3D12Object* object, const std::string_view name)
	{
		if (object != nullptr)
		{
			object->SetName(Wider(name).c_str());
		}
	}
} // namespace fe::dx
