#pragma once
#include <Igniter.h>
#include <Core/Event.h>

namespace ig
{
    enum class EFileTrackerResult
    {
        Success,
        DuplicateTrackingRequest,
        EmptyEvent,
        NotDirectoryPath,
        FileDoesNotExist,
        FailedToOpen,
        FailedToCreateIOEventHandle,
    };

    enum class ETrackingFilterFlags : uint32_t
    {
        Default = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE,
        ChangeFileName = FILE_NOTIFY_CHANGE_FILE_NAME,
        ChangeDirName = FILE_NOTIFY_CHANGE_DIR_NAME,
        ChangeAttributes = FILE_NOTIFY_CHANGE_ATTRIBUTES,
        ChangeSize = FILE_NOTIFY_CHANGE_SIZE,
        ChangeLastWrite = FILE_NOTIFY_CHANGE_LAST_WRITE,
        ChangeLastAccess = FILE_NOTIFY_CHANGE_LAST_ACCESS,
        ChangeCreation = FILE_NOTIFY_CHANGE_CREATION,
        ChangeSecurity = FILE_NOTIFY_CHANGE_SECURITY
    };

    inline constexpr ETrackingFilterFlags operator|(const ETrackingFilterFlags lhs, const ETrackingFilterFlags rhs)
    {
        using UnderlyingType = std::underlying_type_t<ETrackingFilterFlags>;
        return static_cast<ETrackingFilterFlags>(static_cast<UnderlyingType>(lhs) |
                                                 static_cast<UnderlyingType>(rhs));
    }

    enum class EFileTrackingAction
    {
        Unknown,
        Added,          /* 파일이 디렉터리에 추가됨 */
        Removed,        /* 파일이 디렉터리에서 삭제됨 */
        Modified,       /* 파일이 수정 됨 */
        RenamedOldName, /* 이름이 변경된 파일의 변경 전 이름 */
        RenamedNewName  /* 이름이 변경된 파일의 변경 후 이름 */
    };

    enum class EFileTrackingMode
    {
        Polling,
        Event
    };

    struct [[nodiscard]] FileNotification
    {
    public:
        bool operator==(const FileNotification& other) { return Action == other.Action && Path == other.Path; }

    public:
        EFileTrackingAction Action = EFileTrackingAction::Unknown;
        fs::path Path{};
    };

    class AsyncFileTracker
    {
    public:
        struct Iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = FileNotification;
            using difference_type = size_t;
            using reference = FileNotification&;

        public:
            Iterator() = default;
            Iterator(AsyncFileTracker& fileTracker) noexcept;
            Iterator(std::default_sentinel_t) noexcept;
            Iterator(Iterator&&) noexcept = default;
            ~Iterator() = default;

            FileNotification& operator*() noexcept;
            const FileNotification& operator*() const noexcept;

            auto* operator->() noexcept;
            const auto* operator->() const noexcept;

            Iterator& operator++();
            Iterator operator++(int);

            [[nodiscard]] friend bool operator==(const Iterator& lhs, std::default_sentinel_t)
            {
                return !lhs.notification.has_value();
            }

        private:
            AsyncFileTracker* fileTracker = nullptr;
            std::optional<FileNotification> notification{ std::nullopt };
        };

    public:
        AsyncFileTracker() = default;
        ~AsyncFileTracker();

        [[nodiscard]] bool IsTracking() const noexcept { return directoryHandle != INVALID_HANDLE_VALUE; }
        [[nodiscard]] EFileTrackerResult StartTracking(const fs::path& targetDirPath,
                                                       const EFileTrackingMode trackingMode = EFileTrackingMode::Polling,
                                                       const ETrackingFilterFlags filter = ETrackingFilterFlags::Default,
                                                       const bool bTrackingRecursively = true,
                                                       const chrono::milliseconds ioCheckingPeriod = 33ms);
        void StopTracking();
        std::optional<FileNotification> TryGetNotification();

        Iterator begin() { return Iterator{ *this }; }
        std::default_sentinel_t end() { return std::default_sentinel_t{}; }

        Event<String, FileNotification>& GetEvent()
        {
            IG_CHECK(!IsTracking() && "Modify event during tracking is not thread safe!");
            return event;
        }

    private:
        fs::path path{};
        HANDLE directoryHandle{ INVALID_HANDLE_VALUE };
        HANDLE ioEventHandle{ INVALID_HANDLE_VALUE };

        std::jthread trackingThread{};
        std::promise<void> ioPromise{};

        constexpr static uint32_t ReservedRawBufferSizeInBytes = 1024Ui32 * 1024Ui32; /* 1 KB */
        std::vector<uint8_t> rawBuffer{ std::vector<uint8_t>(ReservedRawBufferSizeInBytes) };

        EFileTrackingMode mode{ EFileTrackingMode::Polling };
        Event<String, FileNotification> event{};
        concurrency::concurrent_queue<FileNotification> notificationQueue{};
    };
} // namespace ig