#include "Frieren/Frieren.h"
#include "Igniter/Core/String.h"
#include "Igniter/Core/FrameManager.h"
#include "Igniter/Core/Window.h"
#include "Igniter/Input/InputManager.h"
#include "Igniter/D3D12/ShaderBlob.h"
#include "Igniter/D3D12/RootSignature.h"
#include "Igniter/D3D12/DescriptorHeap.h"
#include "Igniter/Asset/AssetManager.h"
#include "Igniter/Render/RenderPass.h"
#include "Igniter/Render/RenderContext.h"
#include "Igniter/Render/TempConstantBufferAllocator.h"
#include "Igniter/Render/RenderGraphBuilder.h"
#include "Igniter/Render/Utils.h"
#include "Igniter/Component/CameraArchetype.h"
#include "Igniter/Component/CameraComponent.h"
#include "Igniter/Component/NameComponent.h"
#include "Igniter/Component/StaticMeshComponent.h"
#include "Igniter/Component/TransformComponent.h"
#include "Igniter/Gameplay/World.h"
#include "Igniter/ImGui/ImGuiCanvas.h"
#include "Igniter/ImGui/ImGuiExtensions.h"
#include "Frieren/Render/RendererPrototype.h"
#include "Frieren/Game/Component/FpsCameraController.h"
#include "Frieren/Game/System/TestGameSystem.h"
#include "Frieren/ImGui/EditorCanvas.h"
#include "Frieren/Application/TestApp.h"

namespace fe
{
    class MainRenderPass final : public ig::RenderPass
    {
    public:
        struct Output
        {
            ig::RGResourceHandle RenderOutput;
            ig::RGResourceHandle DepthStencilBuffer;
        };

    public:
        MainRenderPass(ig::RenderContext& renderContext, ig::Window& window, ig::TempConstantBufferAllocator& tempConstantBufferAllocator)
            : renderContext(renderContext)
            , mainViewport(window.GetViewport())
            , tempConstantBufferAllocator(tempConstantBufferAllocator)
            , world(world)
            , ig::RenderPass("MainRenderPass"_fs)
        {
            /* #sy_note 테스트 코드! 원래 이런식으로 생성 하는건 비 효율적!  */
            ig::RenderDevice& renderDevice = renderContext.GetRenderDevice();
            rootSignature = ig::MakePtr<ig::RootSignature>(renderDevice.CreateBindlessRootSignature().value());

            const ig::ShaderCompileDesc vsDesc{.SourcePath = "Assets/Shader/BasicVertexShader.hlsl"_fs,
                .Type = ig::EShaderType::Vertex,
                .OptimizationLevel = ig::EShaderOptimizationLevel::None};

            const ig::ShaderCompileDesc psDesc{.SourcePath = "Assets/Shader/BasicPixelShader.hlsl"_fs, .Type = ig::EShaderType::Pixel};

            vertexShader = ig::MakePtr<ig::ShaderBlob>(vsDesc);
            pixelShader = ig::MakePtr<ig::ShaderBlob>(psDesc);

            ig::GraphicsPipelineStateDesc psoDesc;
            psoDesc.SetVertexShader(*vertexShader);
            psoDesc.SetPixelShader(*pixelShader);
            psoDesc.SetRootSignature(*rootSignature);
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            pso = renderContext.CreatePipelineState(psoDesc);
        }

        ~MainRenderPass() override
        {
            for (ig::LocalFrameIndex localFrameIdx : ig::views::iota(0Ui8, ig::NumFramesInFlight))
            {
                renderContext.DestroyGpuView(rtvs[localFrameIdx]);
                renderContext.DestroyGpuView(dsvs[localFrameIdx]);
            }

            renderContext.DestroyPipelineState(pso);
        }

        void Setup(ig::RenderGraphBuilder& builder) override
        {
            ig::GpuTextureDesc renderOutputDesc{};
            renderOutputDesc.AsRenderTarget(
                static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), 1, DXGI_FORMAT_R8G8B8A8_UNORM);
            renderOutputDesc.DebugName = "MainRenderPassOutput"_fs;
            output.RenderOutput = builder.WriteTexture(builder.CreateTexture(renderOutputDesc), D3D12_BARRIER_LAYOUT_RENDER_TARGET);

            ig::GpuTextureDesc depthStencilDesc;
            depthStencilDesc.DebugName = "DepthStencilBuffer"_fs;
            depthStencilDesc.AsDepthStencil(
                static_cast<uint32_t>(mainViewport.width), static_cast<uint32_t>(mainViewport.height), DXGI_FORMAT_D32_FLOAT);
            depthStencilDesc.InitialLayout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
            output.DepthStencilBuffer = builder.WriteTexture(builder.CreateTexture(depthStencilDesc), D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE);
        }

        void PostCompile(ig::RenderGraph& renderGraph) override
        {
            renderTarget = renderGraph.GetTexture(output.RenderOutput);
            depthStencil = renderGraph.GetTexture(output.DepthStencilBuffer);

            for (ig::LocalFrameIndex localFrameIdx : ig::views::iota(0Ui8, ig::NumFramesInFlight))
            {
                rtvs[localFrameIdx] =
                    renderContext.CreateRenderTargetView(renderTarget.Resources[localFrameIdx], D3D12_TEX2D_RTV{.MipSlice = 0, .PlaneSlice = 0});
                dsvs[localFrameIdx] = renderContext.CreateDepthStencilView(depthStencil.Resources[localFrameIdx], D3D12_TEX2D_DSV{.MipSlice = 0});
            }
        }

        void Execute([[maybe_unused]] tf::Subflow& renderPassSubflow, eastl::vector<ig::CommandContext*>& pendingCmdCtxList) override
        {
            struct BasicRenderResources
            {
                uint32_t VertexBufferIdx;
                uint32_t PerFrameBufferIdx;
                uint32_t PerObjectBufferIdx;
                uint32_t DiffuseTexIdx;
                uint32_t DiffuseTexSamplerIdx;
            };

            struct PerFrameBuffer
            {
                ig::Matrix ViewProj{};
            };

            struct PerObjectBuffer
            {
                ig::Matrix LocalToWorld{};
            };

            const ig::LocalFrameIndex localFrameIdx = ig::FrameManager::GetLocalFrameIndex();
            ig::Registry& registry = world->GetRegistry();
            ig::TempConstantBuffer perFrameConstantBuffer = tempConstantBufferAllocator.Allocate<PerFrameBuffer>(localFrameIdx);

            PerFrameBuffer perFrameBuffer{};
            auto cameraView = registry.view<ig::CameraComponent, ig::TransformComponent>();
            IG_CHECK(cameraView.size_hint() == 1);
            for (auto [entity, camera, transformData] : cameraView.each())
            {
                /* #sy_todo Multiple Camera, Render Target per camera */
                /* Column Vector: PVM; Row Vector: MVP  */
                const ig::Matrix viewMatrix = transformData.CreateView();
                const ig::Matrix projMatrix = camera.CreatePerspective();
                perFrameBuffer.ViewProj = ig::ConvertToShaderSuitableForm(viewMatrix * projMatrix);
            }
            perFrameConstantBuffer.Write(perFrameBuffer);

            auto renderCmdCtx = renderContext.GetMainGfxCommandContextPool().Request(localFrameIdx, "MainRenderPass"_fs);
            renderCmdCtx->Begin(renderContext.Lookup(pso));
            {
                auto bindlessDescHeaps = renderContext.GetBindlessDescriptorHeaps();
                renderCmdCtx->SetDescriptorHeaps(bindlessDescHeaps);
                renderCmdCtx->SetRootSignature(*rootSignature);

                ig::GpuView& rtv = *renderContext.Lookup(rtvs[localFrameIdx]);
                renderCmdCtx->ClearRenderTarget(rtv);
                ig::GpuView& dsv = *renderContext.Lookup(dsvs[localFrameIdx]);
                renderCmdCtx->ClearDepth(dsv);
                renderCmdCtx->SetRenderTarget(rtv, dsv);
                renderCmdCtx->SetViewport(mainViewport);
                renderCmdCtx->SetScissorRect(mainViewport);
                renderCmdCtx->SetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                ig::AssetManager& assetManager = ig::Igniter::GetAssetManager();

                const auto renderableView = registry.view<ig::StaticMeshComponent, ig::TransformComponent>();
                ig::GpuView* perFrameCbvPtr = renderContext.Lookup(perFrameConstantBuffer.GetConstantBufferView());
                for (auto [entity, staticMeshComponent, transform] : renderableView.each())
                {
                    if (staticMeshComponent.Mesh)
                    {
                        ig::StaticMesh* staticMeshPtr = assetManager.Lookup(staticMeshComponent.Mesh);
                        ig::GpuBuffer* indexBufferPtr = renderContext.Lookup(staticMeshPtr->GetIndexBuffer());
                        ig::GpuView* vertexBufferSrvPtr = renderContext.Lookup(staticMeshPtr->GetVertexBufferSrv());
                        renderCmdCtx->SetIndexBuffer(*indexBufferPtr);
                        {
                            ig::TempConstantBuffer perObjectConstantBuffer = tempConstantBufferAllocator.Allocate<PerObjectBuffer>(localFrameIdx);
                            ig::GpuView* perObjectCBViewPtr = renderContext.Lookup(perObjectConstantBuffer.GetConstantBufferView());
                            const auto perObjectBuffer =
                                PerObjectBuffer{.LocalToWorld = ig::ConvertToShaderSuitableForm(transform.CreateTransformation())};
                            perObjectConstantBuffer.Write(perObjectBuffer);

                            /* #sy_todo 각각의 Material이나 Diffuse가 Invalid 하다면 Engine Default로 fallback 될 수 있도록 조치 */
                            ig::Material* materialPtr = assetManager.Lookup(staticMeshPtr->GetMaterial());
                            ig::Texture* diffuseTexPtr = assetManager.Lookup(materialPtr->GetDiffuse());
                            ig::GpuView* diffuseTexSrvPtr = renderContext.Lookup(diffuseTexPtr->GetShaderResourceView());
                            ig::GpuView* diffuseTexSamplerPtr = renderContext.Lookup(diffuseTexPtr->GetSampler());

                            const BasicRenderResources params{.VertexBufferIdx = vertexBufferSrvPtr->Index,
                                .PerFrameBufferIdx = perFrameCbvPtr->Index,
                                .PerObjectBufferIdx = perObjectCBViewPtr->Index,
                                .DiffuseTexIdx = diffuseTexSrvPtr->Index,
                                .DiffuseTexSamplerIdx = diffuseTexSamplerPtr->Index};

                            renderCmdCtx->SetRoot32BitConstants(0, params, 0);
                        }

                        const ig::StaticMesh::Desc& snapshot = staticMeshPtr->GetSnapshot();
                        const ig::StaticMesh::LoadDesc& loadDesc = snapshot.LoadDescriptor;
                        renderCmdCtx->DrawIndexed(loadDesc.NumIndices);
                    }
                }
            }
            renderCmdCtx->End();
            pendingCmdCtxList.emplace_back((ig::CommandContext*) renderCmdCtx);
        }

        Output GetOutput() const { return output; }

        void SetWorld(ig::World* newWorld) { world = newWorld; }

    private:
        ig::RenderContext& renderContext;
        ig::Viewport mainViewport;
        ig::TempConstantBufferAllocator& tempConstantBufferAllocator;
        Output output;

        ig::RGTexture renderTarget;
        ig::RGTexture depthStencil;

        eastl::array<ig::Handle<ig::GpuView, ig::RenderContext>, ig::NumFramesInFlight> rtvs;
        eastl::array<ig::Handle<ig::GpuView, ig::RenderContext>, ig::NumFramesInFlight> dsvs;

        ig::Ptr<ig::RootSignature> rootSignature;
        ig::Ptr<ig::ShaderBlob> vertexShader;
        ig::Ptr<ig::ShaderBlob> pixelShader;

        ig::Handle<ig::PipelineState, ig::RenderContext> pso;

        ig::World* world;
    };

    class ImGuiPass : public ig::RenderPass
    {
    public:
        struct Input
        {
            ig::RGResourceHandle FinalRenderOutput;
        };

        struct Output
        {
            ig::RGResourceHandle GuiRenderOutput;
        };

    public:
        ImGuiPass(ig::RenderContext& renderContext, ig::Window& window, const Input input)
            : renderContext(renderContext), window(window), input(input), ig::RenderPass("ImGuiPass"_fs)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            (void) io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.Fonts->AddFontFromFileTTF("Fonts/D2Coding-ligature.ttf", 18, nullptr, io.Fonts->GetGlyphRangesKorean());
            ig::ImGuiX::SetupDefaultTheme();

            imguiSrv = renderContext.CreateGpuView(ig::EGpuViewType::ShaderResourceView);
            ig::GpuView* imguiSrvPtr = renderContext.Lookup(imguiSrv);
            ig::RenderDevice& renderDevice = renderContext.GetRenderDevice();
            ImGui_ImplWin32_Init(window.GetNative());
            ImGui_ImplDX12_Init(&renderDevice.GetNative(), ig::NumFramesInFlight, DXGI_FORMAT_R8G8B8A8_UNORM,
                &renderContext.GetCbvSrvUavDescriptorHeap().GetNative(), imguiSrvPtr->CPUHandle, imguiSrvPtr->GPUHandle);
        }

        ~ImGuiPass() override
        {
            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            renderContext.DestroyGpuView(imguiSrv);

            for (const ig::LocalFrameIndex localFrameIdx : ig::views::iota(0Ui8, ig::NumFramesInFlight))
            {
                renderContext.DestroyGpuView(guiRenderOutputRtv.Resources[localFrameIdx]);
            }
        }

        void Setup(ig::RenderGraphBuilder& builder) override
        {
            output.GuiRenderOutput = builder.WriteTexture(input.FinalRenderOutput, D3D12_BARRIER_LAYOUT_RENDER_TARGET);
        }

        void PostCompile(ig::RenderGraph& renderGraph) override
        {
            guiRenderOutput = renderGraph.GetTexture(output.GuiRenderOutput);

            for (const ig::LocalFrameIndex localFrameIdx : ig::views::iota(0Ui8, ig::NumFramesInFlight))
            {
                guiRenderOutputRtv.Resources[localFrameIdx] =
                    renderContext.CreateRenderTargetView(guiRenderOutput.Resources[localFrameIdx], D3D12_TEX2D_RTV{.MipSlice = 0, .PlaneSlice = 0});
            }
        }

        void Execute([[maybe_unused]] tf::Subflow& renderPassSubflow, eastl::vector<ig::CommandContext*>& pendingCmdCtxList) override
        {
            const ig::LocalFrameIndex localFrameIdx = ig::FrameManager::GetLocalFrameIndex();
            auto cmdCtx = renderContext.GetMainGfxCommandContextPool().Request(localFrameIdx, "ImGuiRenderPass"_fs);
            cmdCtx->Begin();

            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (canvas != nullptr)
            {
                canvas->OnImGui();
            }
            ImGui::Render();

            cmdCtx->SetRenderTarget(*renderContext.Lookup(guiRenderOutputRtv.Resources[localFrameIdx]));
            cmdCtx->SetDescriptorHeap(renderContext.GetCbvSrvUavDescriptorHeap());
            ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), &cmdCtx->GetNative());
            cmdCtx->End();

            pendingCmdCtxList.emplace_back((ig::CommandContext*) cmdCtx);
        }

        Output GetOutput() const { return output; }

        void SetCanvas(ig::ImGuiCanvas* newCanvas) { canvas = newCanvas; }

    private:
        ig::RenderContext& renderContext;
        ig::Window& window;
        Input input;
        Output output;

        ig::RGTexture guiRenderOutput;
        ig::LocalFrameResource<ig::Handle<ig::GpuView, ig::RenderContext>> guiRenderOutputRtv;

        ig::ImGuiCanvas* canvas = nullptr;
        ig::RenderResource<ig::GpuView> imguiSrv{};
    };

    class BackBufferPass : public ig::RenderPass
    {
    public:
        struct Input
        {
            ig::RGResourceHandle RenderOutput;
        };

    public:
        BackBufferPass(ig::RenderContext& renderContext, ig::Swapchain& swapchain, const Input input)
            : renderContext(renderContext), swapchain(swapchain), input(input), ig::RenderPass("BackBufferPass"_fs)
        {
        }
        ~BackBufferPass() override = default;

        void Setup(ig::RenderGraphBuilder& builder) override { builder.ReadTexture(input.RenderOutput, D3D12_BARRIER_LAYOUT_COPY_SOURCE); }

        void PostCompile(ig::RenderGraph& renderGraph) override { renderOutput = renderGraph.GetTexture(input.RenderOutput); }

        void Execute([[maybe_unused]] tf::Subflow& renderPassSubflow, eastl::vector<ig::CommandContext*>& pendingCmdCtxList) override
        {
            const ig::LocalFrameIndex localFrameIdx = ig::FrameManager::GetLocalFrameIndex();
            auto cmdCtx = renderContext.GetMainGfxCommandContextPool().Request(localFrameIdx, "BackBufferCopyPass"_fs);
            cmdCtx->Begin();
            ig::GpuTexture& backBuffer = *renderContext.Lookup(swapchain.GetBackBuffer());
            cmdCtx->AddPendingTextureBarrier(backBuffer, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_COMMON, D3D12_BARRIER_LAYOUT_COPY_DEST);
            cmdCtx->FlushBarriers();

            cmdCtx->CopyTextureSimple(*renderContext.Lookup(renderOutput.Resources[localFrameIdx]), backBuffer);

            cmdCtx->AddPendingTextureBarrier(backBuffer, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_SYNC_NONE, D3D12_BARRIER_ACCESS_NO_ACCESS,
                D3D12_BARRIER_ACCESS_NO_ACCESS, D3D12_BARRIER_LAYOUT_COPY_DEST, D3D12_BARRIER_LAYOUT_COMMON);
            cmdCtx->FlushBarriers();
            cmdCtx->End();
            pendingCmdCtxList.emplace_back((ig::CommandContext*) cmdCtx);
        }

    private:
        ig::RenderContext& renderContext;
        // Swapchain은 다소 특이한 경우
        ig::Swapchain& swapchain;
        Input input;

        ig::RGTexture renderOutput;
    };

    class DummyAsyncComputePass : public ig::RenderPass
    {
    public:
        struct Input
        {
            ig::RGResourceHandle RenderOutput;
        };

    public:
        DummyAsyncComputePass(const ig::String name, const Input input, const bool bShouldCreateResource = false)
            : input(input), bCreateResource(bShouldCreateResource), ig::RenderPass(name)
        {
        }
        ~DummyAsyncComputePass() override = default;

        void Setup(ig::RenderGraphBuilder& builder) override
        {
            builder.ExecuteOnAsyncCompute();
            if (!bCreateResource)
            {
                AfterRead = builder.ReadTexture(input.RenderOutput, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE);
                return;
            }

            ig::GpuTextureDesc newDummyDesc{};
            newDummyDesc.AsTexture2D(32, 32, 1, DXGI_FORMAT_R8_UNORM, true);
            newDummyDesc.InitialLayout = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
            builder.WriteTexture(builder.CreateTexture(newDummyDesc), D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS);
        }

        void PostCompile(ig::RenderGraph&) override {}

        void Execute([[maybe_unused]] tf::Subflow&, eastl::vector<ig::CommandContext*>&) override {}

    private:
        bool bCreateResource = false;
        Input input;

    public:
        ig::RGResourceHandle AfterRead;
    };

    TestApp::TestApp(const ig::AppDesc& desc)
        : taskExecutor(ig::Igniter::GetTaskExecutor())
        , tempConstantBufferAllocator(ig::MakePtr<ig::TempConstantBufferAllocator>(ig::Igniter::GetRenderContext()))
        , Application(desc)
    {
        /* #sy_test 렌더 그래프 테스트 */
        ig::RenderContext& renderContext = ig::Igniter::GetRenderContext();
        ig::RenderGraphBuilder builder{renderContext};
        mainRenderPass = &builder.AddPass<MainRenderPass>(renderContext, ig::Igniter::GetWindow(), *tempConstantBufferAllocator);

        [[maybe_unused]] DummyAsyncComputePass& dummyPass0 =
            builder.AddPass<DummyAsyncComputePass>("Dummy.0"_fs, DummyAsyncComputePass::Input{}, true);
        [[maybe_unused]] DummyAsyncComputePass& dummyPass1 =
            builder.AddPass<DummyAsyncComputePass>("Dummy.1"_fs, DummyAsyncComputePass::Input{}, true);

        // 여기선 Read
        [[maybe_unused]] DummyAsyncComputePass& dummyPass2 = builder.AddPass<DummyAsyncComputePass>(
            "Dummy.2"_fs, DummyAsyncComputePass::Input{.RenderOutput = mainRenderPass->GetOutput().RenderOutput});

        imGuiPass = &builder.AddPass<ImGuiPass>(renderContext, ig::Igniter::GetWindow(), ImGuiPass::Input{.FinalRenderOutput = dummyPass2.AfterRead});

        backBufferPass = &builder.AddPass<BackBufferPass>(
            renderContext, renderContext.GetSwapchain(), BackBufferPass::Input{.RenderOutput = imGuiPass->GetOutput().GuiRenderOutput});
        renderGraph = builder.Compile();

        /* #sy_test 입력 매니저 테스트 */
        ig::InputManager& inputManager = ig::Igniter::GetInputManager();
        inputManager.MapAction("MoveLeft"_fs, ig::EInput::A);
        inputManager.MapAction("MoveRight"_fs, ig::EInput::D);
        inputManager.MapAction("MoveForward"_fs, ig::EInput::W);
        inputManager.MapAction("MoveBackward"_fs, ig::EInput::S);
        inputManager.MapAction("MoveUp"_fs, ig::EInput::MouseRB);
        inputManager.MapAction("MoveDown"_fs, ig::EInput::MouseLB);
        inputManager.MapAction("Sprint"_fs, ig::EInput::Shift);

        inputManager.MapAxis("TurnYaw"_fs, ig::EInput::MouseDeltaX, 1.f);
        inputManager.MapAxis("TurnAxis"_fs, ig::EInput::MouseDeltaY, 1.f);

        inputManager.MapAction(ig::String("TogglePossessCamera"), ig::EInput::Space);
        /********************************/

        ///* #sy_test 에셋 시스템 테스트 + 게임 플레이 프레임워크 테스트 */
        //[[maybe_unused]] AssetManager& assetManager = Igniter::GetAssetManager();
        // Registry& registry = world->GetRegistry();
        // Guid homuraMeshGuids[] = {
        //    Guid{"0c5a8c40-a6d9-4b6a-bcc9-616b3cef8450"},
        //    Guid{"5e6a8a5a-4358-47a9-93e6-0e7ed165dbc0"},
        //    Guid{"ad4921ad-3654-4ce4-a626-6568ce399dba"},
        //    Guid{"9bbce66e-2292-489f-8e6b-c0f9427fff29"},
        //    Guid{"731d273c-c9e1-4ee1-acaa-6be8d4d54853"},
        //    Guid{"de7ff053-c224-4623-bb0f-17372c2ddb82"},
        //    Guid{"f9ed3d69-d906-4b03-9def-16b6b37211e7"},
        //};

        // std::vector<std::future<ManagedAsset<StaticMesh>>> homuraMeshFutures{};
        // for (Guid guid : homuraMeshGuids)
        //{
        //     homuraMeshFutures.emplace_back(std::async(
        //         std::launch::async, [&assetManager](const Guid guid) { return assetManager.Load<StaticMesh>(guid); }, guid));
        // }

        // for (std::future<ManagedAsset<StaticMesh>>& staticMeshFuture : homuraMeshFutures)
        //{
        //     ManagedAsset<StaticMesh> staticMesh{staticMeshFuture.get()};
        //     if (staticMesh)
        //     {
        //         const auto newEntity{registry.create()};
        //         auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
        //         staticMeshComponent.Mesh = staticMesh;

        //        registry.emplace<TransformComponent>(newEntity);

        //        registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
        //    }
        //}

        // std::future<ManagedAsset<StaticMesh>> homuraAxeMeshFutures{
        //     std::async(std::launch::async, [&assetManager]() { return assetManager.LoadStaticMesh(Guid{"b77399f0-98ec-47cb-ad80-e7287d5833c2"});
        //     })};
        //{
        //     ManagedAsset<StaticMesh> staticMesh{homuraAxeMeshFutures.get()};
        //     if (staticMesh)
        //     {
        //         const auto newEntity{registry.create()};
        //         auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
        //         staticMeshComponent.Mesh = std::move(staticMesh);

        //        registry.emplace<TransformComponent>(newEntity);
        //        registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
        //    }
        //}

        // Guid littleTokyoMeshGuids[] = {
        //     Guid{"b7cdf76b-12ca-4476-8c83-bcb0ca77bd5a"},
        //     Guid{"573ad7dc-62af-46b0-8388-690bf473abe3"},
        //     Guid{"52b2cb76-d1fe-4e5f-97b8-0138391bbea4"},
        //     Guid{"7204d4a4-8899-468d-ab40-d32011817c63"},
        //     Guid{"ce3e807e-73a0-44f2-bd68-e5694b438ef7"},
        //     Guid{"05e0214e-ffc8-4d0b-8021-df4ca53652ba"},
        //     Guid{"3ab251d9-de45-4de9-a6a0-db193e65d161"},
        //     Guid{"f1d4855c-19df-4ec7-97ee-54e3cc3fa38b"},
        //     Guid{"21a63b13-4a92-460b-81bf-064e4950b848"},
        //     Guid{"60f75ff6-5e72-4e2f-8d4f-71dd94de5105"},
        //     Guid{"4f1cd7b8-fcd9-4bab-8bf1-0fc1268ccb02"},
        //     Guid{"3834a543-15cc-471c-8a28-2715201777f7"},
        //     Guid{"815e2b44-1ede-44ba-a8b5-d20ccc44d7f2"},
        //     Guid{"43c9dcec-28df-45cc-a881-e154b1bc38d8"},
        //     Guid{"40a98f09-f5d2-4944-ab41-1ce1da88c888"},
        //     Guid{"2d2625b8-27d7-43cb-8895-af5164bcf0f9"},
        //     Guid{"7da574d2-5d54-43fe-be78-b394dcb5a689"},
        //     Guid{"70f20985-0a11-4b4c-bdc6-30adb116080c"},
        //     Guid{"657015c8-8d4b-496a-9ba5-54c5a9d6aa70"},
        //     Guid{"fcb4a3d5-2c54-41bb-9a25-1e8ef20cb7f6"},
        //     Guid{"cd08f473-47d9-4e53-b998-be27b28d1d6b"},
        //     Guid{"f8dad50c-7331-42a4-be15-72923762a76a"},
        // };

        // std::vector<std::future<ManagedAsset<StaticMesh>>> littleTokyoMeshFutures{};
        // for (Guid guid : littleTokyoMeshGuids)
        //{
        //     littleTokyoMeshFutures.emplace_back(std::async(
        //         std::launch::async, [&assetManager](const Guid guid) { return assetManager.LoadStaticMesh(guid); }, guid));
        // }

        // for (std::future<ManagedAsset<StaticMesh>>& staticMeshFuture : littleTokyoMeshFutures)
        //{
        //     ManagedAsset<StaticMesh> staticMesh{staticMeshFuture.get()};
        //     if (staticMesh)
        //     {
        //         const auto newEntity{registry.create()};
        //         auto& staticMeshComponent{registry.emplace<StaticMeshComponent>(newEntity)};
        //         staticMeshComponent.Mesh = staticMesh;

        //        registry.emplace<TransformComponent>(newEntity);
        //        registry.emplace<NameComponent>(newEntity, assetManager.Lookup(staticMeshComponent.Mesh)->GetSnapshot().Info.GetVirtualPath());
        //    }
        //}

        // const Entity cameraEntity = CameraArchetype::Create(registry);
        //{
        //     registry.emplace<NameComponent>(cameraEntity, "Camera"_fs);

        //    TransformComponent& cameraTransform = registry.get<TransformComponent>(cameraEntity);
        //    cameraTransform.Position = Vector3{0.f, 0.f, -30.f};

        //    CameraComponent& cameraComponent = registry.get<CameraComponent>(cameraEntity);
        //    cameraComponent.CameraViewport = Igniter::GetWindow().GetViewport();

        //    registry.emplace<FpsCameraController>(cameraEntity);
        //}
        ///********************************************/

        /* #sy_test ImGui 통합 테스트 */
        editorCanvas = MakePtr<EditorCanvas>(*this);
        ig::Igniter::SetImGuiCanvas(static_cast<ig::ImGuiCanvas*>(editorCanvas.get()));
        /************************************/

        /* #sy_test Game Mode 타입 메타 정보 테스트 */
        gameSystem =
            std::move(*entt::resolve<TestGameSystem>().func(ig::meta::GameSystemConstructFunc).invoke({}).try_cast<ig::Ptr<ig::GameSystem>>());
        /************************************/

        // Json dumpedWorld{};
        //[[maybe_unused]] std::string worldDumpTest = world->Serialize(dumpedWorld).dump();
        // Igniter::GetAssetManager().Import("TestMap"_fs, {.WorldToSerialize = world.get()});
        // world = MakePtr<World>();
        // world->Deserialize(dumpedWorld);
        ig::AssetManager& assetManager = ig::Igniter::GetAssetManager();
        world = ig::MakePtr<ig::World>(assetManager, assetManager.Load<ig::Map>(ig::Guid{"92d1aad6-7d75-41a4-be10-c9f8bfdb787e"}));
        mainRenderPass->SetWorld(world.get());
        imGuiPass->SetCanvas(editorCanvas.get());
    }

    TestApp::~TestApp() {}

    void TestApp::Update(const float deltaTime)
    {
        gameSystem->Update(deltaTime, *world);
    }

    void TestApp::PreRender(const ig::LocalFrameIndex localFrameIdx)
    {
        tempConstantBufferAllocator->Reset(localFrameIdx);
    }

    ig::GpuSync TestApp::Render(const ig::LocalFrameIndex)
    {
        return renderGraph->Execute(taskExecutor);
    }

    void TestApp::PostRender([[maybe_unused]] const ig::LocalFrameIndex)
    {
        static std::once_flag once{};
        std::call_once(once, [this]() { std::cout << renderGraph->Dump(); });
    }

    void TestApp::SetGameSystem(ig::Ptr<ig::GameSystem> newGameSystem)
    {
        gameSystem = std::move(newGameSystem);
    }
}    // namespace fe