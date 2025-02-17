#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    /*
     * 렌더 패스 실행 흐름
     * RenderPass.Init() (Constructor) (RenderPass 에서 시작되는 리소스 생성 및 초기화)
     * // Render Loop begin
     * RenderPass.SetParams(ParamStructure) (외부 리소스 설정; ex. CommandBuffer, Buffer, Texture, etc..)
     * RenderPass.Execute()
     *   if activated
     *      RenderPass.PreExecute()
     *      RenderPass.OnExecute()
     *      RenderPass.PostRender()
     *   else
     *      RenderPass.ExecuteWhenInactive()
     * // Render Loop end
     * RenderPass.Deinit() (Destructor)
     */
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        [[nodiscard]] bool IsActivated() const { return bIsActivated; }
        void SetActivated(const bool bIsPassActivated) { bIsActivated = bIsPassActivated; }

        virtual void Record(const LocalFrameIndex localFrameIdx) final;

    protected:
        virtual void PreRecord([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}
        virtual void OnRecord(const LocalFrameIndex localFrameIdx) = 0;
        virtual void PostRecord([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}
        virtual void OnResize([[maybe_unused]] const U32 width, [[maybe_unused]] const U32 height) {}

        virtual void RecordWhenInactive([[maybe_unused]] const LocalFrameIndex localFrameIdx) {}

    private:
        bool bIsActivated = true;
    };
} // namespace ig
