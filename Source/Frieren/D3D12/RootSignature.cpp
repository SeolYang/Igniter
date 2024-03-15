#include <D3D12/RootSignature.h>
#include <D3D12/RenderDevice.h>

namespace fe
{
    RootSignature::RootSignature(ComPtr<ID3D12RootSignature> newRootSignature)
        : rootSignature(std::move(newRootSignature))
    {
    }

    RootSignature::RootSignature(RootSignature&& other) noexcept
        : RootSignature(std::move(other.rootSignature)) {}

    RootSignature::~RootSignature() {}

    RootSignature& RootSignature::operator=(RootSignature&& other) noexcept
    {
        rootSignature = std::move(other.rootSignature);
        return *this;
    }

} // namespace fe