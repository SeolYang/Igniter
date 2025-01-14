#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuView.h"

namespace ig
{
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/indirect-drawing
    class CommandSignatureDesc
    {
      public:
        CommandSignatureDesc() = default;
        CommandSignatureDesc(const CommandSignatureDesc&) = default;
        CommandSignatureDesc(CommandSignatureDesc&&) noexcept = default;
        ~CommandSignatureDesc() = default;

        CommandSignatureDesc& operator=(const CommandSignatureDesc&) = default;
        CommandSignatureDesc& operator=(CommandSignatureDesc&&) noexcept = default;

        CommandSignatureDesc& AddDrawArgument()
        {
            arguments.emplace_back(D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW});
            return *this;
        }

        CommandSignatureDesc& AddDrawIndexedArgument()
        {
            arguments.emplace_back(D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED});
            return *this;
        }

        CommandSignatureDesc& AddDispatchArgument()
        {
            arguments.emplace_back(D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH});
            return *this;
        }

        CommandSignatureDesc& AddIndexBufferView()
        {
            arguments.emplace_back(D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW});
            return *this;
        }

        CommandSignatureDesc& AddConstant(const U32 rootParameterIdx, const U32 destOffsetIn32BitValues, const U32 num32BitValuesToSet)
        {
            arguments.emplace_back(D3D12_INDIRECT_ARGUMENT_DESC{.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT, .Constant = {rootParameterIdx, destOffsetIn32BitValues, num32BitValuesToSet}});
            return *this;
        }

        CommandSignatureDesc& SetCommandByteStride(const U32 newByteStride)
        {
            IG_CHECK(newByteStride > 0);
            this->byteStride = newByteStride;
            return *this;
        }

        template <typename Ty>
        CommandSignatureDesc& SetCommandByteStride()
        {
            return SetCommandByteStride((U32)sizeof(Ty));
        }

        [[nodiscard]] Option<D3D12_COMMAND_SIGNATURE_DESC> ToCommandSignatureDesc() const
        {
            if (arguments.empty())
            {
                return None<D3D12_COMMAND_SIGNATURE_DESC>();
            }

            D3D12_COMMAND_SIGNATURE_DESC desc{};
            desc.ByteStride = byteStride;
            desc.pArgumentDescs = arguments.data();
            desc.NumArgumentDescs = (U32)arguments.size();
            return desc;
        }

      private:
        Vector<D3D12_INDIRECT_ARGUMENT_DESC> arguments;
        U32 byteStride = 0;
    };
} // namespace ig
