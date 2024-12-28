#include "Frieren/Frieren.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Utils.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Frieren/Render/Renderer.h"

IG_DEFINE_LOG_CATEGORY(Renderer);

namespace fe
{
    struct BasicRenderResources
    {
        uint32_t VertexBufferIdx;
        uint32_t PerFrameBufferIdx;
        uint32_t PerObjectBufferIdx;
        uint32_t DiffuseTexIdx;
        uint32_t DiffuseTexSamplerIdx;
        uint32_t VertexStorageOffset;
    };

    struct PerFrameBuffer
    {
        ig::Matrix ViewProj{ };
    };

    struct PerObjectBuffer
    {
        ig::Matrix LocalToWorld{ };
    };

    Renderer::Renderer(ig::Window& window, ig::RenderContext& renderContext) :\
        renderContext(renderContext),
        mainViewport(window.GetViewport()),
        tempConstantBufferAllocator(ig::MakePtr<ig::TempConstantBufferAllocator>(renderContext))
    {
        ig::GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        bindlessRootSignature    = ig::MakePtr<ig::RootSignature>(gpuDevice.CreateBindlessRootSignature().value());

        const ig::ShaderCompileDesc vsDesc{
            .SourcePath = "Assets/Shaders/BasicVertexShader.hlsl"_fs,
            .Type = ig::EShaderType::Vertex,
            .OptimizationLevel = ig::EShaderOptimizationLevel::None
        };

        const ig::ShaderCompileDesc psDesc{.SourcePath = "Assets/Shaders/BasicPixelShader.hlsl"_fs, .Type = ig::EShaderType::Pixel};

        vs = ig::MakePtr<ig::ShaderBlob>(vsDesc);
        ps = ig::MakePtr<ig::ShaderBlob>(psDesc);

        ig::GraphicsPipelineStateDesc psoDesc;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetPixelShader(*ps);
        psoDesc.SetRootSignature(*bindlessRootSignature);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0]    = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat        = DXGI_FORMAT_D32_FLOAT;
        pso                      = ig::MakePtr<ig::PipelineState>(gpuDevice.CreateGraphicsPipelineState(psoDesc).value());

        ig::GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = "DepthStencilBufferTex"_fs;
        depthStencilDesc.AsDepthStencil(static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), DXGI_FORMAT_D32_FLOAT);

        auto initialCmdCtx = renderContext.GetMainGfxCommandContextPool().Request(0, "Initial Transition"_fs);
        initialCmdCtx->Begin();
        {
            for (const ig::LocalFrameIndex localFrameIdx : ig::views::iota(0Ui8, ig::NumFramesInFlight))
            {
                depthStencils[localFrameIdx] = renderContext.CreateTexture(depthStencilDesc);
                dsvs[localFrameIdx]          = renderContext.CreateDepthStencilView(depthStencils[localFrameIdx], D3D12_TEX2D_DSV{.MipSlice = 0});

                initialCmdCtx->AddPendingTextureBarrier(*renderContext.Lookup(depthStencils[localFrameIdx]),
                                                        D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                                        D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                        D3D12_BARRIER_LAYOUT_UNDEFINED, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
            }

            initialCmdCtx->FlushBarriers();
        }
        initialCmdCtx->End();

        ig::CommandContext* cmdCtxs[1]{(ig::CommandContext*)initialCmdCtx};
        ig::CommandQueue&   mainGfxQueue{renderContext.GetMainGfxQueue()};
        mainGfxQueue.ExecuteContexts(cmdCtxs);
    }

    Renderer::~Renderer()
    {
        for (const auto localFrameIdx : ig::views::iota(0Ui8, ig::NumFramesInFlight))
        {
            renderContext.DestroyGpuView(dsvs[localFrameIdx]);
            renderContext.DestroyTexture(depthStencils[localFrameIdx]);
        }
    }

    void Renderer::PreRender(const ig::LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);
    }

    ig::GpuSyncPoint Renderer::Render(const ig::LocalFrameIndex localFrameIdx)
    {
        if (world == nullptr)
        {
            return ig::GpuSyncPoint::Invalid();
        }

        ig::Registry&          registry               = world->GetRegistry();
        ig::TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameBuffer>(localFrameIdx);

        PerFrameBuffer perFrameBuffer{ };
        auto           cameraView = registry.view<ig::CameraComponent, ig::TransformComponent>();

        ig::Swapchain&     swapchain        = renderContext.GetSwapchain();
        ig::GpuTexture*    backBufferPtr    = renderContext.Lookup(swapchain.GetBackBuffer());
        const ig::GpuView* backBufferRtvPtr = renderContext.Lookup(swapchain.GetRenderTargetView());

        ig::MeshStorage&                      meshStorage                   = ig::Engine::GetMeshStorage();
        const ig::RenderHandle<ig::GpuView>   staticMeshVertexStorageSrv    = meshStorage.GetStaticMeshVertexStorageShaderResourceView();
        const ig::GpuView*                    staticMeshVertexStorageSrvPtr = renderContext.Lookup(staticMeshVertexStorageSrv);
        const ig::RenderHandle<ig::GpuBuffer> vertexIndexStorageBuffer      = meshStorage.GetVertexIndexStorageBuffer();
        ig::GpuBuffer*                        vertexIndexStorageBufferPtr   = renderContext.Lookup(vertexIndexStorageBuffer);

        ig::CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        auto              renderCmdCtx = renderContext.GetMainGfxCommandContextPool().Request(localFrameIdx, "MainGfx"_fs);

        bool bRenderable = false;
        for (auto [entity, camera, transformData] : cameraView.each())
        {
            /* #sy_todo Multiple Camera, Render Target per camera */
            /* Column Vector: PVM; Row Vector: MVP  */
            if (!camera.bIsMainCamera)
            {
                continue;
            }

            if (camera.CameraViewport.width < 1.f || camera.CameraViewport.height < 1.f)
            {
                continue;
            }

            const ig::Matrix viewMatrix = transformData.CreateView();
            const ig::Matrix projMatrix = camera.CreatePerspective();
            perFrameBuffer.ViewProj     = ig::ConvertToShaderSuitableForm(viewMatrix * projMatrix);
            perFrameConstantBuffer.Write(perFrameBuffer);
            bRenderable = true;
        }

        renderCmdCtx->Begin(pso.get());
        {
            auto bindlessDescHeaps = renderContext.GetBindlessDescriptorHeaps();
            renderCmdCtx->SetDescriptorHeaps(bindlessDescHeaps);
            renderCmdCtx->SetRootSignature(*bindlessRootSignature);

            renderCmdCtx->AddPendingTextureBarrier(*backBufferPtr,
                                                   D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
                                                   D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
                                                   D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
            renderCmdCtx->FlushBarriers();

            renderCmdCtx->ClearRenderTarget(*backBufferRtvPtr);
            ig::GpuView& dsv = *renderContext.Lookup(dsvs[localFrameIdx]);
            renderCmdCtx->ClearDepth(dsv);
            renderCmdCtx->SetRenderTarget(*backBufferRtvPtr, dsv);
            renderCmdCtx->SetViewport(mainViewport);
            renderCmdCtx->SetScissorRect(mainViewport);
            renderCmdCtx->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            renderCmdCtx->SetIndexBuffer(*vertexIndexStorageBufferPtr);

            if (bRenderable)
            {
                ig::AssetManager& assetManager   = ig::Engine::GetAssetManager();
                const auto        renderableView = registry.view<ig::StaticMeshComponent, ig::TransformComponent>();
                ig::GpuView*      perFrameCbvPtr = renderContext.Lookup(perFrameConstantBuffer.GetConstantBufferView());
                for (auto [entity, staticMeshComponent, transform] : renderableView.each())
                {
                    if (staticMeshComponent.Mesh)
                    {
                        ig::StaticMesh*                             staticMeshPtr    = assetManager.Lookup(staticMeshComponent.Mesh);
                        const ig::MeshStorage::Space<ig::VertexSM>* vertexSpace      = meshStorage.Lookup(staticMeshPtr->GetVertexSpace());
                        const ig::MeshStorage::Space<ig::U32>*      vertexIndexSpace = meshStorage.Lookup(staticMeshPtr->GetVertexIndexSpace());
                        {
                            ig::TempConstantBuffer perObjectConstantBuffer = tempConstantBufferAllocator->Allocate<PerObjectBuffer>(localFrameIdx);
                            const ig::GpuView*     perObjectCbvPtr         = renderContext.Lookup(perObjectConstantBuffer.GetConstantBufferView());
                            const auto             perObjectBuffer         = PerObjectBuffer{.LocalToWorld = ig::ConvertToShaderSuitableForm(transform.CreateTransformation())};
                            perObjectConstantBuffer.Write(perObjectBuffer);

                            /* #sy_todo 각각의 Material이나 Diffuse가 Invalid 하다면 Engine Default로 fallback 될 수 있도록 조치 */
                            ig::MaterialAsset* materialPtr          = assetManager.Lookup(staticMeshPtr->GetMaterial());
                            ig::Texture*  diffuseTexPtr        = assetManager.Lookup(materialPtr->GetDiffuse());
                            ig::GpuView*  diffuseTexSrvPtr     = renderContext.Lookup(diffuseTexPtr->GetShaderResourceView());
                            ig::GpuView*  diffuseTexSamplerPtr = renderContext.Lookup(diffuseTexPtr->GetSampler());

                            const BasicRenderResources params{
                                .VertexBufferIdx = staticMeshVertexStorageSrvPtr->Index,
                                .PerFrameBufferIdx = perFrameCbvPtr->Index,
                                .PerObjectBufferIdx = perObjectCbvPtr->Index,
                                .DiffuseTexIdx = diffuseTexSrvPtr->Index,
                                .DiffuseTexSamplerIdx = diffuseTexSamplerPtr->Index,
                                .VertexStorageOffset = static_cast<ig::U32>(vertexSpace->Allocation.OffsetIndex)
                            };

                            renderCmdCtx->SetRoot32BitConstants(0, params, 0);
                        }

                        renderCmdCtx->DrawIndexed((ig::U32)vertexIndexSpace->Allocation.NumElements, (ig::U32)vertexIndexSpace->Allocation.OffsetIndex);
                    }
                }
            }

            renderCmdCtx->AddPendingTextureBarrier(*backBufferPtr,
                                                   D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
                                                   D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                   D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
            renderCmdCtx->FlushBarriers();
        }
        renderCmdCtx->End();

        ig::CommandContext* renderCmdCtxPtrs[] = {(ig::CommandContext*)renderCmdCtx};
        mainGfxQueue.ExecuteContexts(renderCmdCtxPtrs);
        return mainGfxQueue.MakeSyncPointWithSignal();
    }
} // namespace fe
