#pragma once
#include "Igniter/Igniter.h"

namespace ig
{
    class RenderPass
    {
    public:
        virtual ~RenderPass() = default;

        [[nodiscard]] bool IsActivated() const { return bIsActivated; }
        void               SetActivated(const bool bIsPassActivated) { bIsActivated = bIsPassActivated; }

        virtual void Execute(const LocalFrameIndex localFrameIdx) final;

    protected:
        virtual void PreRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) { }
        virtual void Render(const LocalFrameIndex localFrameIdx) = 0;
        virtual void PostRender([[maybe_unused]] const LocalFrameIndex localFrameIdx) { }

        virtual void ExecuteWhenInactive([[maybe_unused]] const LocalFrameIndex localFrameIdx) { }

    private:
        bool bIsActivated = true;
    };
} // namespace ig
