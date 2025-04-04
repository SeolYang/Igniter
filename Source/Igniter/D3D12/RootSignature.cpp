#include "Igniter/Igniter.h"
#include "Igniter/D3D12/RootSignature.h"

namespace ig
{
    RootSignature::RootSignature(ComPtr<ID3D12RootSignature> newRootSignature)
        : rootSignature(std::move(newRootSignature))
    {}

    RootSignature::RootSignature(RootSignature&& other) noexcept
        : RootSignature(std::move(other.rootSignature))
    {}

    RootSignature::~RootSignature() {}

    RootSignature& RootSignature::operator=(RootSignature&& other) noexcept
    {
        rootSignature = std::move(other.rootSignature);
        return *this;
    }
} // namespace ig
