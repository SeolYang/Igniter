#pragma once
#include <D3D12/Common.h>

namespace fe::dx
{
	class Device;
	// 현재는 Bindless root signature 의 생성만 구현, 이후에 Root Parameter, Root Constant 또는 Root Descriptor Table로
	// 확장 가능하도록 일반화
	class RootSignature
	{
	public:
		RootSignature(Device& device);
		~RootSignature();

		ID3D12RootSignature& GetNative() { return *rootSignature.Get(); }

	private:
		ComPtr<ID3D12RootSignature> rootSignature;
	};
} // namespace fe::dx
