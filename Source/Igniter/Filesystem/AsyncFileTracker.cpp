#include <Igniter.h>
#include <Filesystem/AsyncFileTracker.h>

namespace ig
{
    AsyncFileTracker::~AsyncFileTracker()
    {
        StopTracking();
    }

    EFileTrackerStatus AsyncFileTracker::StartTracking(const fs::path& targetDirPath,
                                                       const EFileTrackingMode trackingMode,
                                                       const ETrackingFilterFlags filter,
                                                       const bool bTrackingRecursively,
                                                       const uint64_t,
                                                       const chrono::milliseconds)
    {
        if (IsTracking())
        {
            return EFileTrackerStatus::DuplicateTrackingRequest;
        }

        if (trackingMode == EFileTrackingMode::Event && !event.HasAnySubscriber())
        {
            return EFileTrackerStatus::EmptyEvent;
        }

        if (!fs::is_directory(targetDirPath))
        {
            return EFileTrackerStatus::NotDirectoryPath;
        }

        if (!fs::exists(targetDirPath))
        {
            return EFileTrackerStatus::FileDoesNotExist;
        }

        constexpr DWORD DesiredAccess = GENERIC_READ;
        constexpr DWORD ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        constexpr DWORD CreationDepositio = OPEN_EXISTING;

        IG_CHECK(directoryHandle == INVALID_HANDLE_VALUE);
        directoryHandle = CreateFile(targetDirPath.c_str(),
                                     DesiredAccess, ShareMode, nullptr, CreationDepositio, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);

        if (directoryHandle == INVALID_HANDLE_VALUE)
        {
            return EFileTrackerStatus::FailedToOpen;
        }

        mode = trackingMode;
        path = targetDirPath;
        const auto notifyFilter = static_cast<DWORD>(filter);

        trackingThread = std::jthread(
            [this, bTrackingRecursively, notifyFilter](std::stop_token stopToken)
            {
                WCHAR fileNameBuffer[MAX_PATH]{ 0 };

                OVERLAPPED overlapped{ 0 };
                overlapped.hEvent = CreateEvent(nullptr, 0, 0, nullptr);

                while (!stopToken.stop_requested())
                {
                    IG_CHECK(directoryHandle != INVALID_HANDLE_VALUE);
                    const bool bRequestSucceeded = ReadDirectoryChangesExW(directoryHandle,
                                                                           rawBuffer.data(), ReservedRawBufferSizeInBytes,
                                                                           bTrackingRecursively, notifyFilter,
                                                                           nullptr, &overlapped, nullptr, ReadDirectoryNotifyExtendedInformation);

                    if (bRequestSucceeded)
                    {
                        while (!stopToken.stop_requested())
                        {
                            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, 0);
                            if (waitResult != WAIT_OBJECT_0)
                            {
                                continue;
                            }

                            DWORD transferedBytes{ 0 };
                            GetOverlappedResult(directoryHandle, &overlapped, &transferedBytes, FALSE);
                            const auto* notifyInfo = reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(rawBuffer.data());
                            while (transferedBytes > 0)
                            {
                                FileNotification newNotification;
                                switch (notifyInfo->Action)
                                {
                                    case FILE_ACTION_ADDED:
                                        newNotification.Action = EFileTrackingAction::Added;
                                        break;

                                    case FILE_ACTION_REMOVED:
                                        newNotification.Action = EFileTrackingAction::Removed;
                                        break;

                                    case FILE_ACTION_MODIFIED:
                                        newNotification.Action = EFileTrackingAction::Modified;
                                        break;
                                    case FILE_ACTION_RENAMED_OLD_NAME:
                                        newNotification.Action = EFileTrackingAction::RenamedOldName;
                                        break;

                                    case FILE_ACTION_RENAMED_NEW_NAME:
                                        newNotification.Action = EFileTrackingAction::RenamedNewName;
                                        break;

                                    default:
                                        continue;
                                }

                                if (FAILED(StringCbCopyNW(fileNameBuffer, sizeof(fileNameBuffer), notifyInfo->FileName, notifyInfo->FileNameLength)))
                                {
                                    continue;
                                }

                                newNotification.Path = path / fileNameBuffer;
                                newNotification.CreationTime = notifyInfo->CreationTime.QuadPart;
                                newNotification.LastModificationTime = notifyInfo->LastModificationTime.QuadPart;
                                newNotification.LastChangeTime = notifyInfo->LastChangeTime.QuadPart;
                                newNotification.LastAccessTime = notifyInfo->LastAccessTime.QuadPart;
                                newNotification.FileSize = notifyInfo->FileSize.QuadPart;

                                if (mode == EFileTrackingMode::Event)
                                {
                                    event.Notify(newNotification);
                                }
                                else
                                {
                                    notificationQueue.push(std::move(newNotification));
                                }

                                if (notifyInfo->NextEntryOffset > 0)
                                {
                                    notifyInfo = reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(reinterpret_cast<const uint8_t*>(notifyInfo) + notifyInfo->NextEntryOffset);
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                    else
                    {
                        IG_UNIMPLEMENTED();
                    }
                }

                IG_CHECK(overlapped.hEvent != INVALID_HANDLE_VALUE);
                CloseHandle(overlapped.hEvent);
            });

        return EFileTrackerStatus::Success;
    }

    void AsyncFileTracker::StopTracking()
    {
        if (IsTracking())
        {
            CancelIo(directoryHandle);
            trackingThread.request_stop();
            trackingThread.join();

            IG_CHECK(directoryHandle != INVALID_HANDLE_VALUE);
            CloseHandle(directoryHandle);
            directoryHandle = INVALID_HANDLE_VALUE;

            notificationQueue.clear();
            path.clear();
        }
    }

    std::optional<FileNotification> AsyncFileTracker::TryGetNotification()
    {
        IG_CHECK(IsTracking() && mode == EFileTrackingMode::Default);
        FileNotification notification;
        if (notificationQueue.try_pop(notification))
        {
            return std::make_optional(notification);
        }

        return std::nullopt;
    }

    AsyncFileTracker::Iterator::Iterator(AsyncFileTracker& fileTracker) noexcept : fileTracker(&fileTracker)
    {
        ++(*this);
    }

    AsyncFileTracker::Iterator::Iterator(std::default_sentinel_t) noexcept
    {
    }

    FileNotification& AsyncFileTracker::Iterator::operator*() noexcept
    {
        return *notification;
    }

    const FileNotification& AsyncFileTracker::Iterator::operator*() const noexcept
    {
        return *notification;
    }

    auto* AsyncFileTracker::Iterator::operator->() noexcept
    {
        return &notification;
    }

    const auto* AsyncFileTracker::Iterator::operator->() const noexcept
    {
        return &notification;
    }

    AsyncFileTracker::Iterator& AsyncFileTracker::Iterator::operator++()
    {
        if (fileTracker != nullptr)
        {
            notification = fileTracker->TryGetNotification();
            if (!notification)
            {
                fileTracker = nullptr;
            }
        }

        return *this;
    }

    AsyncFileTracker::Iterator AsyncFileTracker::Iterator::operator++(int)
    {
        Iterator oldItr{};
        oldItr.fileTracker = fileTracker;
        oldItr.notification = std::move(notification);

        ++(*this);
        return oldItr;
    }
} // namespace ig