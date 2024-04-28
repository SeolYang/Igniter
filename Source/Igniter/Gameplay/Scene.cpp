#include <Igniter.h>
#include <Component/CameraComponent.h>
#include <Render/RenderContext.h>
#include <Gameplay/Scene.h>

namespace ig
{
    RenderViewport::RenderViewport(const RenderViewportParams params) : renderContext(params.Context), cameraEntity(params.Camera)
    {
        IG_CHECK(params.Camera != entt::null);
        IG_CHECK(params.InitialWidth > 0.f && params.InitialHeight > 0.f);
        Resize(static_cast<uint32_t>(params.InitialWidth), static_cast<uint32_t>(params.InitialHeight));
    }

    RenderViewport::~RenderViewport()
    {
        for (const uint8_t locaclFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            renderContext.DestroyGpuView(shaderResourceViews[locaclFrameIdx]);
            renderContext.DestroyGpuView(renderTargetViews[locaclFrameIdx]);
            renderContext.DestroyTexture(renderTargets[locaclFrameIdx]);
        }
    }

    void RenderViewport::Resize(const uint32_t newWidth, const uint32_t newHeight)
    {
        IG_CHECK(newWidth > 0.f && newHeight > 0.f);
        if (width != newWidth || height != newHeight)
        {
            width = newWidth;
            height = newHeight;

            GpuTextureDesc resizedTexDesc{};
            /* #sy_todo Swapchain 에서 사용하는 backbuffer format과 일치 시킬 것! */
            resizedTexDesc.AsRenderTarget(width, height, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
            for (const uint8_t localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
            {
                renderContext.DestroyTexture(renderTargets[localFrameIdx]);
                renderContext.DestroyGpuView(renderTargetViews[localFrameIdx]);
                renderContext.DestroyGpuView(shaderResourceViews[localFrameIdx]);

                const auto newRenderTarget = renderContext.CreateTexture(resizedTexDesc);
                const auto newRtv = renderContext.CreateRenderTargetView(newRenderTarget, D3D12_TEX2D_RTV{0, 0});
                const auto newSrv = renderContext.CreateShaderResourceView(
                    newRenderTarget, D3D12_TEX2D_SRV{.MostDetailedMip = 0, .MipLevels = 1, .PlaneSlice = 0, .ResourceMinLODClamp = 0.f});
                renderTargets[localFrameIdx] = newRenderTarget;
                renderTargetViews[localFrameIdx] = newRtv;
                shaderResourceViews[localFrameIdx] = newSrv;
            }
        }
    }

    Scene::Scene(RenderContext& renderContext) : renderContext(renderContext)
    {
        registry.on_construct<CameraComponent>().connect<&Scene::OnCameraConstruct>(*this);
        registry.on_destroy<CameraComponent>().connect<&Scene::OnCameraDestroy>(*this);
    }

    void Scene::Update()
    {
        // per frame.. camera data update for resizing..?
        const auto cameraView = registry.view<CameraComponent>();
        for (const Entity entity : cameraView)
        {
            const auto itr = eastl::find_if(renderViewports.begin(), renderViewports.end(),
                [entity](RenderViewport& renderViewport) { return entity == renderViewport.GetCameraEntity(); });
            if (itr != renderViewports.end())
            {
                const auto& cameraComponent = registry.get<CameraComponent>(entity);
                itr->Resize(
                    static_cast<uint32_t>(cameraComponent.CameraViewport.width), static_cast<uint32_t>(cameraComponent.CameraViewport.height));
            }
        }
    }

    void Scene::OnCameraConstruct(Registry& owner, const Entity entity)
    {
        IG_CHECK(entity != entt::null && &owner == &registry);
        /* #sy_note 만약에 Camera Component 내에서 Viewport 값이 변경 된다면? */
        const auto& cameraComponent = owner.get<CameraComponent>(entity);
        const RenderViewportParams params{renderContext, entity, cameraComponent.CameraViewport.width, cameraComponent.CameraViewport.height};
        renderViewports.emplace_back(params);
    }

    void Scene::OnCameraDestroy([[maybe_unused]] Registry& owner, const Entity entity)
    {
        IG_CHECK(entity != entt::null && &owner == &registry);
        eastl::erase_if(renderViewports, [entity](RenderViewport& renderViewport) { return entity == renderViewport.GetCameraEntity(); });
    }
}    // namespace ig