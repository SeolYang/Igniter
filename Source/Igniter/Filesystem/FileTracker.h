#pragma once
#include <Core/Win32API.h>
#include <Core/Event.h>
#include <Filesystem/Common.h>
#include <thread>
#include <future>

namespace ig
{
    enum class EFileTrackerResult
    {
        Success,
        DuplicateTrackingRequest,
        NotDirectoryPath,
        FileDoesNotExist,
        FailedToOpen,
        FailedToCreateIOEventHandle
    };

    enum class ETrackingFilterFlags : uint32_t
    {
        ChangeFileName = FILE_NOTIFY_CHANGE_FILE_NAME,
        ChangeDirName = FILE_NOTIFY_CHANGE_DIR_NAME,
        ChangeAttributes = FILE_NOTIFY_CHANGE_ATTRIBUTES,
        ChangeSize = FILE_NOTIFY_CHANGE_SIZE,
        ChangeLastWrite = FILE_NOTIFY_CHANGE_LAST_WRITE,
        ChangeLastAccess = FILE_NOTIFY_CHANGE_LAST_ACCESS,
        ChangeCreation = FILE_NOTIFY_CHANGE_CREATION,
        ChangeSecurity = FILE_NOTIFY_CHANGE_SECURITY
    };

    inline ETrackingFilterFlags operator|(const ETrackingFilterFlags lhs, const ETrackingFilterFlags rhs)
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
        RenamedOldName, /* 파일 이름이 바뀌었으나, 이전 이름과 동일함. */
        RenamedNewName  /* 파일 이름이 바뀌었으며, 이전 이름과 다른 새 이름임. */
    };

    struct FileNotification
    {
    public:
        EFileTrackingAction Action = EFileTrackingAction::Unknown;
        fs::path Path{};
    };

    class FileTracker
    {
    public:
        FileTracker() = default;
        ~FileTracker();

        [[nodiscard]] bool IsTracking() const noexcept { return directoryHandle != INVALID_HANDLE_VALUE; }
        [[nodiscard]] EFileTrackerResult StartTracking(const fs::path& targetDirPath,
                                                       const ETrackingFilterFlags filter =  ETrackingFilterFlags::ChangeDirName | ETrackingFilterFlags::ChangeFileName | ETrackingFilterFlags::ChangeSize,
                                                       const bool bTrackingRecursively = true,
                                                       const chrono::milliseconds ioCheckingPeriod = 66ms);
        void StopTracking();
        std::optional<FileNotification> TryGetNotification();

    private:
        std::jthread trackingThread{};

        fs::path path{};
        HANDLE directoryHandle{ INVALID_HANDLE_VALUE };
        HANDLE ioEventHandle{ INVALID_HANDLE_VALUE };

        std::promise<void> ioPromise{};

        concurrency::concurrent_queue<FileNotification> notificationQueue{};

        constexpr static uint32_t ReservedBufferSizeInBytes = 1024Ui32 * 1024Ui32; /* 1 KB */
        std::vector<uint8_t> buffer{ std::vector<uint8_t>(ReservedBufferSizeInBytes) };
    };
} // namespace ig