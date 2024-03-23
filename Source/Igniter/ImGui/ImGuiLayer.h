#pragma once
#include <Igniter.h>

namespace ig
{
    class ImGuiLayer
    {
    public:
        ImGuiLayer()
        {
            /* #sy_improvement atomically? */
            static uint64_t LayerCounter = 0;
            id = LayerCounter++;
        }

        virtual ~ImGuiLayer() = default;

        virtual void Render() = 0;

        bool IsVisible() const { return bIsVisible; }
        void SetVisibility(const bool bIsVisibleLayer) { bIsVisible = bIsVisibleLayer; }

        uint64_t GetID() const { return id; }

        uint64_t GetPriority() const { return priority; }
        void SetPriority(const uint64_t newPriority) { priority = newPriority; }

    protected:
        bool bIsVisible = false;

    private:
        uint64_t id = 0;
        /* lower value = high priority */
        uint64_t priority = 0;

    };
} // namespace ig