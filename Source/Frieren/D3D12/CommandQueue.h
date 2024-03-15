#pragma once
#include <D3D12/Common.h>
#include <Core/Container.h>
#include <Core/Mutex.h>

namespace fe
{
    class GpuSync final
    {
        friend class CommandQueue;

    public:
        GpuSync() = default;
        GpuSync(const GpuSync&) = default;
        GpuSync(GpuSync&&) noexcept = default;
        ~GpuSync() = default;

        GpuSync& operator=(const GpuSync& rhs) = default;
        GpuSync& operator=(GpuSync&&) noexcept = default;

        bool operator<(const GpuSync& rhs) const
        {
            return (fence != nullptr && (fence == rhs.fence)) &&
                   syncPoint < rhs.syncPoint;
        }

        bool operator>(const GpuSync& rhs) const
        {
            return (fence != nullptr && (fence == rhs.fence)) &&
                   syncPoint > rhs.syncPoint;
        }

        bool operator<=(const GpuSync& rhs) const
        {
            return (fence != nullptr && (fence == rhs.fence)) &&
                   syncPoint <= rhs.syncPoint;
        }

        bool operator>=(const GpuSync& rhs) const
        {
            return (fence != nullptr && (fence == rhs.fence)) &&
                   syncPoint >= rhs.syncPoint;
        }

        bool operator==(const GpuSync& rhs) const
        {
            return (fence != nullptr && (fence == rhs.fence)) &&
                   syncPoint == rhs.syncPoint;
        }

        bool operator!=(const GpuSync& rhs) const
        {
            return fence != rhs.fence;
        }

        [[nodiscard]] bool IsValid() const { return fence != nullptr && syncPoint > 0; }
        [[nodiscard]] operator bool() const { return IsValid(); }

        [[nodiscard]] size_t GetSyncPoint() const { return syncPoint; }
        [[nodiscard]] size_t GetCompletedSyncPoint() const
        {
            if (fence != nullptr)
            {
                return fence->GetCompletedValue();
            }

            checkNoEntry();
            return 0;
        }
        [[nodiscard]] bool IsExpired() const { return syncPoint <= GetCompletedSyncPoint(); }

        void WaitOnCpu()
        {
            check(IsValid());
            if (fence != nullptr && syncPoint > GetCompletedSyncPoint())
            {
                fence->SetEventOnCompletion(syncPoint, nullptr);
            }
        }

    private:
        GpuSync(ID3D12Fence& fence, const size_t syncPoint)
            : fence(&fence),
              syncPoint(syncPoint)
        {
            check(IsValid());
        }

        [[nodiscard]] ID3D12Fence& GetFence()
        {
            check(fence != nullptr);
            return *fence;
        }

    private:
        ID3D12Fence* fence = nullptr;
        size_t syncPoint = 0;
    };

    class RenderDevice;
    class CommandContext;
    class CommandQueue final
    {
        friend class RenderDevice;

    public:
        CommandQueue(const CommandQueue&) = delete;
        CommandQueue(CommandQueue&& other) noexcept;
        ~CommandQueue();

        CommandQueue& operator=(const CommandQueue&) = delete;
        CommandQueue& operator=(CommandQueue&&) noexcept = delete;

        bool IsValid() const { return native; }
        operator bool() const { return IsValid(); }

        auto& GetNative() { return *native.Get(); }
        EQueueType GetType() const { return type; }

        void AddPendingContext(CommandContext& cmdCtx);
        GpuSync Submit();
        void SyncWith(GpuSync& sync);
        GpuSync Flush();

    private:
        CommandQueue(ComPtr<ID3D12CommandQueue> newNativeQueue, const EQueueType specifiedType, ComPtr<ID3D12Fence> newFence);

        GpuSync FlushUnsafe();

    private:
        static constexpr size_t RecommendedMinNumCommandContexts = 15;

    private:
        ComPtr<ID3D12CommandQueue> native;
        const EQueueType type;

        SharedMutex mutex;
        ComPtr<ID3D12Fence> fence;
        uint64_t syncCounter = 0;
        std::vector<std::reference_wrapper<CommandContext>> pendingContexts;
    };
} // namespace fe