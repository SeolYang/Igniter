#pragma once
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class GpuDevice;

    /* 루트 시그니처 제한 사항: https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits */
    class RootSignature final
    {
        friend class GpuDevice;

    public:
        RootSignature(const RootSignature&) = delete;
        RootSignature(RootSignature&& other) noexcept;
        ~RootSignature();

        RootSignature& operator=(const RootSignature&) = delete;
        RootSignature& operator=(RootSignature&& other) noexcept;

        bool IsValid() const { return rootSignature; }
        operator bool() const { return IsValid(); }

        [[nodiscard]] auto& GetNative() { return *rootSignature.Get(); }

    private:
        RootSignature(ComPtr<ID3D12RootSignature> newRootSignature);

    private:
        ComPtr<ID3D12RootSignature> rootSignature;
    };
} // namespace ig
