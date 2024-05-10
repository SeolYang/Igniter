#pragma once
#include "Igniter/Core/Handle.h"
#include "Igniter/D3D12/Common.h"
#include "Igniter/D3D12/GpuTextureDesc.h"
#include "Igniter/D3D12/GpuBufferDesc.h"
#include "Igniter/Render/RenderContext.h"

namespace ig
{
    class GpuTexture;
    class GpuBuffer;
    class RenderContext;
}    // namespace ig

namespace ig::experimental
{
    class RenderPass;

    using RGBuffer = LocalFrameResource<Handle<GpuBuffer, RenderContext>>;
    using RGTexture = LocalFrameResource<Handle<GpuTexture, RenderContext>>;

    struct RGResourceHandle final
    {
    public:
        inline explicit operator uint64_t() const noexcept
        {
            return Index | (static_cast<uint64_t>(Version) >> 16) | (static_cast<uint64_t>(Subresource) >> 32);
        }

        [[nodiscard]] bool operator==(const RGResourceHandle& other) const noexcept
        {
            return Index == other.Index && Version == other.Version && Subresource == other.Subresource;
        }

        [[nodiscard]] inline RGResourceHandle MakeVersionLess() const noexcept { return RGResourceHandle{Index, 0, Subresource}; }

    public:
        uint16_t Index = 0;
        uint16_t Version = 0;
        uint32_t Subresource = 0;
    };

    struct RGTextureSubresouce final
    {
        uint32_t MipSlice = 0;
        uint32_t ArraySlice = 0;
    };

    namespace details
    {
        enum class ERGResourceType : uint8_t
        {
            Buffer,
            Texture
        };

        struct RGResource final
        {
        public:
            const GpuBufferDesc& GetBufferDesc(const RenderContext& renderContext) const
            {
                IG_CHECK(Type == ERGResourceType::Buffer);
                if (bIsExternal)
                {
                    return renderContext.Lookup(std::get<RGBuffer>(Resource).Resources[0])->GetDesc();
                }

                return std::get<GpuBufferDesc>(Resource);
            }

            const GpuTextureDesc& GetTextureDesc(const RenderContext& renderContext) const
            {
                IG_CHECK(Type == ERGResourceType::Texture);
                if (bIsExternal)
                {
                    return renderContext.Lookup(std::get<RGTexture>(Resource).Resources[0])->GetDesc();
                }

                return std::get<GpuTextureDesc>(Resource);
            }

        public:
            ERGResourceType Type = ERGResourceType::Buffer;
            bool bIsExternal = false;
            std::variant<GpuTextureDesc, GpuBufferDesc, RGTexture, RGBuffer> Resource{};
            D3D12_BARRIER_LAYOUT InitialLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;    // Buffer 대상으론 항상 Undefined
        };

        enum class ERGExecutableQueue
        {
            MainGfx,
            AsyncCompute,
            AsyncCopy
        };

        struct RGRenderPassPackage
        {
            Ptr<RenderPass> RenderPassPtr{};
            UnorderedMap<RGResourceHandle, D3D12_BARRIER_LAYOUT> ReadDependencies{};
            UnorderedMap<RGResourceHandle, D3D12_BARRIER_LAYOUT> WriteDependencies{};
            ERGExecutableQueue ExecuteOn = ERGExecutableQueue::MainGfx;
            bool bHasNonExternalReadDependency = false;
        };

        struct RGDependencyLevel
        {
            uint16_t Level = 0;
            eastl::vector<size_t> MainGfxQueueRenderPasses{};
            eastl::vector<size_t> AsyncComputeQueueRenderPasses{};
            eastl::vector<size_t> AsyncCopyQueueRenderPasses{};
            eastl::vector<size_t> RenderPasses{};
        };

        struct RGLayoutTransition
        {
            RGResourceHandle TextureHandle{};
            D3D12_BARRIER_LAYOUT Before{D3D12_BARRIER_LAYOUT_UNDEFINED};
            D3D12_BARRIER_LAYOUT After{D3D12_BARRIER_LAYOUT_UNDEFINED};
        };

        struct RGSyncPoint
        {
            bool bSyncWithAsyncComputeQueue = false;
            bool bSyncWithAsyncCopyQueue = false;
            eastl::vector<RGLayoutTransition> LayoutTransitions{};
        };
    }

}    // namespace ig::experimental

template <>
struct std::hash<ig::experimental::RGResourceHandle>
{
public:
    inline size_t operator()(const ig::experimental::RGResourceHandle& handle) const noexcept { return (uint64_t) handle; }
};