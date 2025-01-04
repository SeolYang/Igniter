#include "Igniter/Igniter.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Core/Log.h"
#include "Igniter/Core/Engine.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/PipelineState.h"
#include "Igniter/D3D12/PipelineStateDesc.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/GpuBuffer.h"
#include "Igniter/D3D12/GpuTexture.h"
#include "Igniter/D3D12/CommandSignature.h"
#include "Igniter/D3D12/CommandSignatureDesc.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/Utils.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/MeshStorage.h"
#include "Igniter/Render/SceneProxy.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Render/Renderer.h"

IG_DEFINE_LOG_CATEGORY(RendererLog);

namespace ig
{
    struct DrawRenderableCommand
    {
        U32 PerFrameConstantBuffer = IG_NUMERIC_MAX_OF(PerFrameConstantBuffer);
        U32 RenderableIndex = IG_NUMERIC_MAX_OF(RenderableIndex);
        D3D12_INDEX_BUFFER_VIEW IndexBufferView; // 64bit inter = uint2
        D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments;
    };

    struct PerFrameBuffer
    {
        Matrix ViewProj{};
        Vector3 CameraPosition{};
        float Padding0;
        Vector3 CameraForward{};
        float Padding1;

        U32 StaticMeshVertexStorage = IG_NUMERIC_MAX_OF(StaticMeshVertexStorage);
        U32 TransformStorage = IG_NUMERIC_MAX_OF(TransformStorage);
        U32 MaterialStorage = IG_NUMERIC_MAX_OF(MaterialStorage);
        U32 RenderableStorage = IG_NUMERIC_MAX_OF(RenderableStorage);
        U32 RenderableIndices = IG_NUMERIC_MAX_OF(RenderableIndices);
        U32 NumMaxRenderables = IG_NUMERIC_MAX_OF(NumMaxRenderables);

        U32 PerFrameConstantBuffer = IG_NUMERIC_MAX_OF(PerFrameConstantBuffer);

        U64 IndexBufferLocation = IG_NUMERIC_MAX_OF(IndexBufferLocation);
        U32 IndexBufferSize = IG_NUMERIC_MAX_OF(IndexBufferSize);
        U32 IndexBufferFormat = IG_NUMERIC_MAX_OF(IndexBufferFormat);
        // TODO Light Sttorage/Indices/NumMaxLights
    };

    struct ComputeCullingConstants
    {
        U32 PerFrameConstantBuffer;
        U32 DrawCommandStorageUav;
    };

    Renderer::Renderer(const Window& window, RenderContext& renderContext, const MeshStorage& meshStorage, const SceneProxy& sceneProxy) :
        window(&window), renderContext(&renderContext), meshStorage(&meshStorage), sceneProxy(&sceneProxy), mainViewport(window.GetViewport()), tempConstantBufferAllocator(MakePtr<TempConstantBufferAllocator>(renderContext))
    {
        GpuDevice& gpuDevice = renderContext.GetGpuDevice();
        bindlessRootSignature = MakePtr<RootSignature>(gpuDevice.CreateBindlessRootSignature().value());

        const ShaderCompileDesc vsDesc{
            .SourcePath = "Assets/Shaders/BasicVertexShader.hlsl"_fs,
            .Type = EShaderType::Vertex,
            .OptimizationLevel = EShaderOptimizationLevel::None};

        const ShaderCompileDesc psDesc{.SourcePath = "Assets/Shaders/BasicPixelShader.hlsl"_fs, .Type = EShaderType::Pixel};

        vs = MakePtr<ShaderBlob>(vsDesc);
        ps = MakePtr<ShaderBlob>(psDesc);

        GraphicsPipelineStateDesc psoDesc;
        psoDesc.SetVertexShader(*vs);
        psoDesc.SetPixelShader(*ps);
        psoDesc.SetRootSignature(*bindlessRootSignature);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pso = MakePtr<PipelineState>(gpuDevice.CreateGraphicsPipelineState(psoDesc).value());

        GpuTextureDesc depthStencilDesc;
        depthStencilDesc.DebugName = "DepthStencilBufferTex"_fs;
        depthStencilDesc.AsDepthStencil(static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), DXGI_FORMAT_D32_FLOAT);

        auto initialCmdList = renderContext.GetMainGfxCommandListPool().Request(0, "Initial Transition"_fs);
        initialCmdList->Open();
        {
            for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
            {
                depthStencils[localFrameIdx] = renderContext.CreateTexture(depthStencilDesc);
                dsvs[localFrameIdx] = renderContext.CreateDepthStencilView(depthStencils[localFrameIdx], D3D12_TEX2D_DSV{.MipSlice = 0});

                initialCmdList->AddPendingTextureBarrier(*renderContext.Lookup(depthStencils[localFrameIdx]),
                                                         D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE,
                                                         D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_NO_ACCESS,
                                                         D3D12_BARRIER_LAYOUT_UNDEFINED, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
            }

            initialCmdList->FlushBarriers();
        }
        initialCmdList->Close();

        CommandList* cmdLists[]{initialCmdList};
        CommandQueue& mainGfxQueue{renderContext.GetMainGfxQueue()};
        mainGfxQueue.ExecuteContexts(cmdLists);

        GpuBufferDesc uavCounterResetBufferDesc{};
        uavCounterResetBufferDesc.AsUploadBuffer(GpuBufferDesc::kUavCounterSize);
        uavCounterResetBufferDesc.DebugName = "UavCounterResetBuffer"_fs;
        uavCounterResetBuffer = MakePtr<GpuBuffer>(gpuDevice.CreateBuffer(uavCounterResetBufferDesc).value());
        auto const mappedUavCounterResetBuffer = reinterpret_cast<GpuBufferDesc::UavCounter*>(uavCounterResetBuffer->Map());
        ZeroMemory(mappedUavCounterResetBuffer, sizeof(GpuBufferDesc::kUavCounterSize));
        uavCounterResetBuffer->Unmap();

        const ShaderCompileDesc computeCullingShaderDesc{.SourcePath = "Assets/Shaders/ComputeCulling.hlsl"_fs, .Type = EShaderType::Compute};
        computeCullingShader = MakePtr<ShaderBlob>(computeCullingShaderDesc);

        ComputePipelineStateDesc computePipelineStateDesc{};
        computePipelineStateDesc.SetComputeShader(*computeCullingShader);
        computePipelineStateDesc.SetRootSignature(*bindlessRootSignature);
        computePipelineStateDesc.Name = "ComputeCullingPipelineState"_fs;
        computeCullingPso = MakePtr<PipelineState>(gpuDevice.CreateComputePipelineState(computePipelineStateDesc).value());
        IG_CHECK(computeCullingPso->IsCompute());

        CommandSignatureDesc commandSignatureDesc{};
        commandSignatureDesc.AddConstant(0, 0, 2)
            .AddIndexBufferView()
            .AddDrawIndexedArgument()
            .SetCommandByteStride<DrawRenderableCommand>();
        commandSignature = MakePtr<CommandSignature>(gpuDevice.CreateCommandSignature("IndirectCommandSignature", commandSignatureDesc, *bindlessRootSignature).value());

        for (const LocalFrameIndex localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            drawCmdStorage[localFrameIdx] = MakePtr<GpuStorage>(
                renderContext,
                String(std::format("DrawRenderableCmdBuffer.{}", localFrameIdx)),
                (U32)sizeof(DrawRenderableCommand),
                kInitNumDrawCommands,
                true, true, true);
        }

        mainGfxQueue.MakeSyncPointWithSignal(renderContext.GetMainGfxFence()).WaitOnCpu();
    }

    Renderer::~Renderer()
    {
        for (const auto localFrameIdx : views::iota(0Ui8, NumFramesInFlight))
        {
            drawCmdStorage[localFrameIdx]->ForceReset();
            renderContext->DestroyGpuView(dsvs[localFrameIdx]);
            renderContext->DestroyTexture(depthStencils[localFrameIdx]);
        }
    }

    void Renderer::PreRender(const LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);

        if (drawCmdSpace[localFrameIdx].IsValid())
        {
            drawCmdStorage[localFrameIdx]->Deallocate(drawCmdSpace[localFrameIdx]);
            // 블럭을 합쳐서 fragmentation 최소화
            drawCmdStorage[localFrameIdx]->ForceReset();
        }

        drawCmdSpace[localFrameIdx] =
            drawCmdStorage[localFrameIdx]->Allocate(sceneProxy->GetNumMaxRenderables(localFrameIdx));
    }

    GpuSyncPoint Renderer::Render([[maybe_unused]] const LocalFrameIndex localFrameIdx)
    {
        return GpuSyncPoint::Invalid();
        // Registry& registry = world->GetRegistry();
        // TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameBuffer>(localFrameIdx);

        // PerFrameBuffer perFrameBuffer{};
        // auto cameraView = registry.view<CameraComponent, TransformComponent>();

        // Swapchain& swapchain = renderContext->GetSwapchain();
        // GpuTexture* backBufferPtr = renderContext->Lookup(swapchain.GetBackBuffer());
        // const GpuView* backBufferRtvPtr = renderContext->Lookup(swapchain.GetRenderTargetView());

        // MeshStorage& meshStorage = Engine::GetMeshStorage();
        // const RenderHandle<GpuView> staticMeshVertexStorageSrv = meshStorage.GetStaticMeshVertexStorageShaderResourceView();
        // const GpuView* staticMeshVertexStorageSrvPtr = renderContext->Lookup(staticMeshVertexStorageSrv);
        // const RenderHandle<GpuBuffer> vertexIndexStorageBuffer = meshStorage.GetVertexIndexStorageBuffer();
        // GpuBuffer* vertexIndexStorageBufferPtr = renderContext->Lookup(vertexIndexStorageBuffer);

        // CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        // auto renderCmdList = renderContext->GetMainGfxCommandListPool().Request(localFrameIdx, "MainGfx"_fs);

        // bool bRenderable = false;
        // for (auto [entity, camera, transformData] : cameraView.each())
        //{
        //     /* #sy_todo Multiple Camera, Render Target per camera */
        //     /* Column Vector: PVM; Row Vector: MVP  */
        //     if (!camera.bIsMainCamera)
        //     {
        //         continue;
        //     }

        //    if (camera.CameraViewport.width < 1.f || camera.CameraViewport.height < 1.f)
        //    {
        //        continue;
        //    }

        //    const Matrix viewMatrix = transformData.CreateView();
        //    const Matrix projMatrix = camera.CreatePerspective();
        //    perFrameBuffer.ViewProj = ConvertToShaderSuitableForm(viewMatrix * projMatrix);
        //    perFrameConstantBuffer.Write(perFrameBuffer);
        //    bRenderable = true;
        //}

        // renderCmdList->Open(pso.get());
        //{
        //     auto bindlessDescHeaps = renderContext->GetBindlessDescriptorHeaps();
        //     renderCmdList->SetDescriptorHeaps(bindlessDescHeaps);
        //     renderCmdList->SetRootSignature(*bindlessRootSignature);

        //    renderCmdList->AddPendingTextureBarrier(*backBufferPtr,
        //                                            D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_RENDER_TARGET,
        //                                            D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_ACCESS_RENDER_TARGET,
        //                                            D3D12_BARRIER_LAYOUT_PRESENT, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
        //    renderCmdList->FlushBarriers();

        //    renderCmdList->ClearRenderTarget(*backBufferRtvPtr);
        //    GpuView& dsv = *renderContext->Lookup(dsvs[localFrameIdx]);
        //    renderCmdList->ClearDepth(dsv);
        //    renderCmdList->SetRenderTarget(*backBufferRtvPtr, dsv);
        //    renderCmdList->SetViewport(mainViewport);
        //    renderCmdList->SetScissorRect(mainViewport);
        //    renderCmdList->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //    renderCmdList->SetIndexBuffer(*vertexIndexStorageBufferPtr);

        //    if (bRenderable)
        //    {
        //        AssetManager& assetManager = Engine::GetAssetManager();
        //        const auto renderableView = registry.view<StaticMeshComponent, TransformComponent>();
        //        GpuView* perFrameCbvPtr = renderContext->Lookup(perFrameConstantBuffer.GetConstantBufferView());
        //        for (auto [entity, staticMeshComponent, transform] : renderableView.each())
        //        {
        //            if (staticMeshComponent.Mesh)
        //            {
        //                StaticMesh* staticMeshPtr = assetManager.Lookup(staticMeshComponent.Mesh);
        //                const MeshStorage::Space<VertexSM>* vertexSpace = meshStorage.Lookup(staticMeshPtr->GetVertexSpace());
        //                const MeshStorage::Space<U32>* vertexIndexSpace = meshStorage.Lookup(staticMeshPtr->GetVertexIndexSpace());
        //                {
        //                    TempConstantBuffer perObjectConstantBuffer = tempConstantBufferAllocator->Allocate<PerObjectBuffer>(localFrameIdx);
        //                    const GpuView* perObjectCbvPtr = renderContext->Lookup(perObjectConstantBuffer.GetConstantBufferView());
        //                    const auto perObjectBuffer = PerObjectBuffer{.LocalToWorld = ConvertToShaderSuitableForm(transform.CreateTransformation())};
        //                    perObjectConstantBuffer.Write(perObjectBuffer);

        //                    /* #sy_todo 각각의 Material이나 Diffuse가 Invalid 하다면 Engine Default로 fallback 될 수 있도록 조치 */
        //                    Material* materialPtr = assetManager.Lookup(staticMeshPtr->GetMaterial());
        //                    Texture* diffuseTexPtr = assetManager.Lookup(materialPtr->GetDiffuse());
        //                    GpuView* diffuseTexSrvPtr = renderContext->Lookup(diffuseTexPtr->GetShaderResourceView());
        //                    GpuView* diffuseTexSamplerPtr = renderContext->Lookup(diffuseTexPtr->GetSampler());

        //                    const BasicRenderResources params{
        //                        .VertexBufferIdx = staticMeshVertexStorageSrvPtr->Index,
        //                        .PerFrameBufferIdx = perFrameCbvPtr->Index,
        //                        .PerObjectBufferIdx = perObjectCbvPtr->Index,
        //                        .DiffuseTexIdx = diffuseTexSrvPtr->Index,
        //                        .DiffuseTexSamplerIdx = diffuseTexSamplerPtr->Index,
        //                        .VertexStorageOffset = static_cast<U32>(vertexSpace->Allocation.OffsetIndex)};

        //                    renderCmdList->SetRoot32BitConstants(0, params, 0);
        //                }

        //                renderCmdList->DrawIndexed((U32)vertexIndexSpace->Allocation.NumElements, (U32)vertexIndexSpace->Allocation.OffsetIndex);
        //            }
        //        }
        //    }

        //    renderCmdList->AddPendingTextureBarrier(*backBufferPtr,
        //                                            D3D12_BARRIER_SYNC_RENDER_TARGET, D3D12_BARRIER_SYNC_NONE,
        //                                            D3D12_BARRIER_ACCESS_RENDER_TARGET, D3D12_BARRIER_ACCESS_NO_ACCESS,
        //                                            D3D12_BARRIER_LAYOUT_RENDER_TARGET, D3D12_BARRIER_LAYOUT_PRESENT);
        //    renderCmdList->FlushBarriers();
        //}
        // renderCmdList->Close();

        // CommandList* renderCmdListPtrs[] = {(CommandList*)renderCmdList};
        // mainGfxQueue.ExecuteContexts(renderCmdListPtrs);
        // return mainGfxQueue.MakeSyncPointWithSignal(renderContext->GetMainGfxFence());
    }

    GpuSyncPoint Renderer::Render(const LocalFrameIndex localFrameIdx, const World& world, [[maybe_unused]] GpuSyncPoint sceneProxyRepSyncPoint)
    {
        IG_CHECK(renderContext != nullptr);
        IG_CHECK(meshStorage != nullptr);
        IG_CHECK(sceneProxy != nullptr);
        IG_CHECK(sceneProxyRepSyncPoint);

        // 프레임 마다 업데이트 되는 버퍼 데이터 업데이트
        PerFrameBuffer perFrameBuffer{};

        const Registry& registry = world.GetRegistry();
        const auto cameraView = registry.view<const CameraComponent, const TransformComponent>();
        bool bMainCameraExists = false;
        for (auto [entity, camera, transformComponent] : cameraView.each())
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

            const Matrix viewMatrix = transformComponent.CreateView();
            const Matrix projMatrix = camera.CreatePerspective();
            perFrameBuffer.ViewProj = ConvertToShaderSuitableForm(viewMatrix * projMatrix);
            bMainCameraExists = true;
            break;
        }

        if (!bMainCameraExists)
        {
            return GpuSyncPoint::Invalid();
        }

        const GpuView* staticMeshVertexStorageSrv = renderContext->Lookup(meshStorage->GetStaticMeshVertexStorageShaderResourceView());
        IG_CHECK(staticMeshVertexStorageSrv != nullptr && staticMeshVertexStorageSrv->IsValid());
        perFrameBuffer.StaticMeshVertexStorage = staticMeshVertexStorageSrv->Index;

        const GpuView* transformStorageSrv = renderContext->Lookup(sceneProxy->GetTransformStorageShaderResourceView(localFrameIdx));
        IG_CHECK(transformStorageSrv != nullptr && transformStorageSrv->IsValid());
        perFrameBuffer.TransformStorage = transformStorageSrv->Index;

        const GpuView* materialStorageSrv = renderContext->Lookup(sceneProxy->GetMaterialStorageShaderResourceView(localFrameIdx));
        IG_CHECK(materialStorageSrv != nullptr && materialStorageSrv->IsValid());
        perFrameBuffer.MaterialStorage = materialStorageSrv->Index;

        const GpuView* renderableStorageSrv = renderContext->Lookup(sceneProxy->GetRenderableStorageShaderResourceView(localFrameIdx));
        IG_CHECK(renderableStorageSrv != nullptr && renderableStorageSrv->IsValid());
        perFrameBuffer.RenderableStorage = renderableStorageSrv->Index;

        const GpuView* renderableIndicesBufferSrv = renderContext->Lookup(sceneProxy->GetRenderableIndicesBufferShaderResourceView(localFrameIdx));
        IG_CHECK(renderableIndicesBufferSrv != nullptr && renderableIndicesBufferSrv->IsValid());
        perFrameBuffer.RenderableIndices = renderableIndicesBufferSrv->Index;
        perFrameBuffer.NumMaxRenderables = sceneProxy->GetNumMaxRenderables(localFrameIdx);

        TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator->Allocate<PerFrameBuffer>(localFrameIdx);
        const GpuView* perFrameBufferCbv = renderContext->Lookup(perFrameConstantBuffer.GetConstantBufferView());
        IG_CHECK(perFrameBufferCbv != nullptr && perFrameBufferCbv->IsValid());
        perFrameBuffer.PerFrameConstantBuffer = perFrameBufferCbv->Index;

        GpuBuffer* vertexIndexStorageBuffer = renderContext->Lookup(meshStorage->GetVertexIndexStorageBuffer());
        IG_CHECK(vertexIndexStorageBuffer != nullptr);
        const std::optional<D3D12_INDEX_BUFFER_VIEW> ibView = vertexIndexStorageBuffer->GetIndexBufferView();
        IG_CHECK(ibView);
        perFrameBuffer.IndexBufferLocation = ibView->BufferLocation;
        perFrameBuffer.IndexBufferSize = ibView->SizeInBytes;
        perFrameBuffer.IndexBufferFormat = (U32)ibView->Format;

        perFrameConstantBuffer.Write(perFrameBuffer);

        // 여기서 부터 시작!: Compute Shader 에서 indirect command buffer를 채우고
        // ExecuteIndirect로 간단한 렌더링 까지
        // 이때 첫 Compute Shader의 시작 전 SceneProxyReplication 을 기다려야함 (wait)
        // 선행 작업: Indirect Command Signature 생성, Indirect Command Buffer 생성(UAV)
        // GpuBufferDesc를 통해서 UAV Counter 사용 가능하도록 만들기
        CommandQueue& asyncComputeQueue{renderContext->GetAsyncComputeQueue()};
        {
            CommandListPool& asyncComputeCmdListPool{renderContext->GetAsyncComputeCommandListPool()};
            auto computeCullingCmdList = asyncComputeCmdListPool.Request(localFrameIdx, "ComputeCullingCmdList"_fs);
            computeCullingCmdList->Open(computeCullingPso.get());
            auto bindlessDescHeaps = renderContext->GetBindlessDescriptorHeaps();
            computeCullingCmdList->SetDescriptorHeaps(bindlessDescHeaps);
            computeCullingCmdList->SetRootSignature(*bindlessRootSignature);

            GpuBuffer* drawStorageBuffer = renderContext->Lookup(drawCmdStorage[localFrameIdx]->GetGpuBuffer());
            IG_CHECK(drawStorageBuffer != nullptr && drawStorageBuffer->IsValid());
            const GpuBufferDesc& drawStorageBufferDesc = drawStorageBuffer->GetDesc();
            IG_CHECK(drawStorageBufferDesc.IsUavCounterEnabled());
            computeCullingCmdList->CopyBuffer(
                *uavCounterResetBuffer, 0, GpuBufferDesc::kUavCounterSize,
                *drawStorageBuffer, drawStorageBufferDesc.GetUavCounterOffset());

            computeCullingCmdList->AddPendingBufferBarrier(
                *drawStorageBuffer,
                D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_SYNC_COMPUTE_SHADING,
                D3D12_BARRIER_ACCESS_COPY_DEST, D3D12_BARRIER_ACCESS_UNORDERED_ACCESS);
            computeCullingCmdList->FlushBarriers();

            const GpuView* drawCmdStorageUav = renderContext->Lookup(drawCmdStorage[localFrameIdx]->GetUnorderedResourceView());
            IG_CHECK(drawCmdStorageUav != nullptr && drawCmdStorageUav->IsValid());
            const ComputeCullingConstants computeCullingConstants{
                .PerFrameConstantBuffer = perFrameBuffer.PerFrameConstantBuffer,
                .DrawCommandStorageUav = drawCmdStorageUav->Index};
            computeCullingCmdList->SetRoot32BitConstants(0, computeCullingConstants, 0);

            // todo Compute Shader 내부적으로 numthreads를 정하고, numElements%numThreads만큼의
            // 작업은 discard 하도록 하기   
            constexpr U32 kNumThreads = 16;
            [[maybe_unused]] const U32 numThreadGroup = ((U32)drawCmdSpace[localFrameIdx].NumElements - 1) / kNumThreads + 1;
            computeCullingCmdList->Dispatch(numThreadGroup, 1, 1);
            computeCullingCmdList->Close();
            asyncComputeQueue.Wait(sceneProxyRepSyncPoint);

            CommandList* cmdLists[]{computeCullingCmdList};
            asyncComputeQueue.ExecuteContexts(cmdLists);
        }
        GpuSyncPoint computeCullingSync = asyncComputeQueue.MakeSyncPointWithSignal(renderContext->GetAsyncComputeFence());

        [[maybe_unused]] Swapchain& swapchain = renderContext->GetSwapchain();
        [[maybe_unused]] GpuTexture* backBufferPtr = renderContext->Lookup(swapchain.GetBackBuffer());
        [[maybe_unused]] const GpuView* backBufferRtvPtr = renderContext->Lookup(swapchain.GetRenderTargetView());

        [[maybe_unused]] CommandQueue& mainGfxQueue{renderContext->GetMainGfxQueue()};
        [[maybe_unused]] auto renderCmdList = renderContext->GetMainGfxCommandListPool().Request(localFrameIdx, "MainGfx"_fs);

        return computeCullingSync;
    }
} // namespace ig
