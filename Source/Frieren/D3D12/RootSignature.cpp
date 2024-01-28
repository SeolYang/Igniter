#include <D3D12/RootSignature.h>
#include <D3D12/Device.h>

namespace fe::dx
{
	RootSignature::RootSignature(const Device& device)
	{
		// WITH THOSE FLAGS, IT MUST BIND DESCRIPTOR HEAP FIRST BEFORE BINDING ROOT SIGNATURE
		const D3D12_ROOT_SIGNATURE_DESC desc{
			.NumParameters = 0,
			.pParameters = nullptr,
			.NumStaticSamplers = 0,
			.pStaticSamplers = nullptr,
			.Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
		};

		ComPtr<ID3DBlob> rootSignatureBlob;
		ComPtr<ID3DBlob> errorBlob;

		FE_ASSERT(SUCCEEDED(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, rootSignatureBlob.GetAddressOf(), errorBlob.GetAddressOf())));

		ID3D12Device10& nativeDevice = device.GetNative();
		FE_ASSERT(SUCCEEDED(nativeDevice.CreateRootSignature(
			0,
			rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature))));
	}

	RootSignature::~RootSignature()
	{
	}
} // namespace fe::dx