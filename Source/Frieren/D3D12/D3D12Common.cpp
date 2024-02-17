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

	bool IsSupportDescriptor(const EDescriptorHeapType descriptorHeapType, const EDescriptorType descriptorType)
	{
		switch (descriptorHeapType)
		{
			case EDescriptorHeapType::CBV_SRV_UAV:
				return descriptorType == EDescriptorType::ShaderResourceView ||
					   descriptorType == EDescriptorType::ConstantBufferView ||
					   descriptorType == EDescriptorType::UnorderedAccessView;
			case EDescriptorHeapType::Sampler:
				return descriptorType == EDescriptorType::Sampler;
			case EDescriptorHeapType::RTV:
				return descriptorType == EDescriptorType::RenderTargetView;
			case EDescriptorHeapType::DSV:
				return descriptorType == EDescriptorType::DepthStencilView;
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
