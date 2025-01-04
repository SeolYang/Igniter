#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/Common.h"

namespace ig
{
    class CommandSignature
    {
        friend class GpuDevice;

    public:
        CommandSignature(const CommandSignature&) = delete;
        CommandSignature(CommandSignature&&) noexcept = default;
        CommandSignature& operator=(const CommandSignature&) = delete;
        CommandSignature& operator=(CommandSignature&&) noexcept = default;
        ~CommandSignature() = default;

        [[nodiscard]] bool IsValid() const noexcept { return native; }
        [[nodiscard]] operator bool() const noexcept { return native; }

        [[nodiscard]] ID3D12CommandSignature& GetNative() { return *native.Get(); }
        [[nodiscard]] const ID3D12CommandSignature& GetNative() const { return *native.Get(); }

    private:
        explicit CommandSignature(ComPtr<ID3D12CommandSignature> commandSignature)
            : native(std::move(commandSignature))
        {}

    private:
        ComPtr<ID3D12CommandSignature> native;
    };
}
