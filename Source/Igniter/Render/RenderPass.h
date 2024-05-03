#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/D3D12/GpuSync.h"

namespace ig
{
    class World;
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;
        /* Preparation */
        virtual void PreRender(const LocalFrameIndex localFrameIdx) = 0;
        /* Record Commands & Sync & Sbmit */
        virtual GpuSync Render(const LocalFrameIndex localFrameIdx, const std::span<GpuSync> syncs) = 0;
        /* Cleanup */
        virtual void PostRender(const LocalFrameIndex localFrameIdx) = 0;
    };
}    // namespace ig

/* A Rendering Pipeline = Composite of Render Passes */
/* A Renderer = Composite of multiple Rendering Pipelines */
// 렌더 패스의 끝은 항상 '하나의 결과'를 만들어 낸다. (ex. 렌더링 결과물, 시뮬레이션 결과, 화면 조도 평균값)
// 또한 렌더 패스가 GPU에서 실행되기 위해서, 필요하는 결과물들(ex. 라이팅 적용을 위한 GBuffer, 파티클 렌더링을 위한 GPU 시뮬레이션 결과 값,
// Auto Exposure를 위한 화면의 조도 평균값 등)이 있을 경우. 이 결과물이 완전히 만들어 진 다음 렌더 패스가 실행 되는 것을 보장하기 위해서
// 여러 '이전' 렌더 패스들의 종료에 동기화 되어야 한다.
// 가장 최적의 경우, 이런 동기화는 이종 큐 간에만 일어나는 것이 바람직 하다. (같은 큐에 대한 작업은, 그 순서가 보존되기 때문에)
// 이종 큐란, 다른 타입의 큐라기 보다는 같은 타입의 큐라도 서로 다른 커맨드 처리 엔진들을 말한다.