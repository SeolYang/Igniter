#pragma once
#include <Igniter.h>
#include <Core/Handle.h>

namespace ig
{
    class RenderContext;
    struct RenderViewportParams
    {
        RenderContext& Context;
        Entity Camera{entt::null};
        float InitialWidth{0.f};
        float InitialHeight{0.f};
    };

    class RenderViewport final
    {
    public:
        RenderViewport(const RenderViewportParams params);
        RenderViewport(const RenderViewport&) = delete;
        RenderViewport(RenderViewport&&) noexcept = default;
        ~RenderViewport();

        RenderViewport& operator=(const RenderViewport&) = delete;
        RenderViewport& operator=(RenderViewport&&) noexcept = default;

        [[nodiscard]] bool IsValid() const
        {
            if (!HasCameraEntity())
            {
                return false;
            }

            for (const uint8_t locaclFrameIdx : views::iota(0Ui8, NumFramesInFlight))
            {
                if (renderTargets[locaclFrameIdx].IsNull() || renderTargetViews[locaclFrameIdx].IsNull() ||
                    shaderResourceViews[locaclFrameIdx].IsNull())
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool HasCameraEntity() const { return cameraEntity != entt::null; }
        [[nodiscard]] Entity GetCameraEntity() const { return cameraEntity; }
        [[nodiscard]] auto GetRenderTargetTexture(const uint8_t localFrameIdx) const { return renderTargets[localFrameIdx]; }
        [[nodiscard]] auto GetRenderTargetView(const uint8_t localFrameIdx) const { return renderTargetViews[localFrameIdx]; }
        [[nodiscard]] auto GetShaderResourcView(const uint8_t localFrameIdx) const { return shaderResourceViews[localFrameIdx]; }

        void Resize(const uint32_t newWidth, const uint32_t newHeight);

    private:
        RenderContext& renderContext;
        Entity cameraEntity{entt::null};
        eastl::array<Handle<class GpuTexture, class RenderContext>, NumFramesInFlight> renderTargets{};
        eastl::array<Handle<class GpuView, class RenderContext>, NumFramesInFlight> renderTargetViews{};
        eastl::array<Handle<class GpuView, class RenderContext>, NumFramesInFlight> shaderResourceViews{};
        uint32_t width = 0;
        uint32_t height = 0;
    };

    class Scene final
    {
    public:
        Scene(RenderContext& renderContext);
        Scene(const Scene&) = delete;
        Scene(Scene&&) noexcept = default;
        ~Scene() = default;

        Scene& operator=(const Scene&) = delete;
        Scene& operator=(Scene&&) noexcept = default;

        Registry& GetRegistry() { return registry; }
        const eastl::list<RenderViewport>& GetRenderViewports() const { return renderViewports; }

        void Update();

        /* #sy_todo 프로토타이핑 했던 거에 기반해서 구현(런타임 메타 데이타 참고) */
        Json& Serialize(Json& archive);
        const Json& Deserialize(const Json& archive);

    private:
        void OnCameraConstruct(Registry& owner, const Entity entity);
        void OnCameraDestroy(Registry& owner, const Entity entity);

    private:
        RenderContext& renderContext;
        Registry registry{};

        eastl::list<RenderViewport> renderViewports{};
    };
}    // namespace ig
