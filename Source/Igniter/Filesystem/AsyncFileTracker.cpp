#include <Filesystem/AsyncFileTracker.h>
#include <Core/Log.h>
#include <strsafe.h>

namespace ig
{
    AsyncFileTracker::~AsyncFileTracker()
    {
        StopTracking();
    }

    EFileTrackerResult AsyncFileTracker::StartTracking(const fs::path& targetDirPath,
                                                       const ETrackingFilterFlags filter, const bool bTrackingRecursively,
                                                       const chrono::milliseconds ioCheckingPeriod)
    {
        if (IsTracking())
        {
            return EFileTrackerResult::DuplicateTrackingRequest;
        }

        if (!fs::is_directory(targetDirPath))
        {
            return EFileTrackerResult::NotDirectoryPath;
        }

        if (!fs::exists(targetDirPath))
        {
            return EFileTrackerResult::FileDoesNotExist;
        }

        constexpr DWORD DesiredAccess = GENERIC_READ;
        constexpr DWORD ShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
        constexpr DWORD CreationDepositio = OPEN_EXISTING;

        IG_CHECK(directoryHandle == INVALID_HANDLE_VALUE);
        directoryHandle = CreateFile(targetDirPath.c_str(),
                                     DesiredAccess, ShareMode, nullptr, CreationDepositio, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, 0);

        if (directoryHandle == INVALID_HANDLE_VALUE)
        {
            return EFileTrackerResult::FailedToOpen;
        }

        IG_CHECK(ioEventHandle == INVALID_HANDLE_VALUE);
        ioEventHandle = CreateEvent(nullptr, 0, 0, nullptr);
        if (ioEventHandle == INVALID_HANDLE_VALUE)
        {
            return EFileTrackerResult::FailedToCreateIOEventHandle;
        }

        path = targetDirPath;
        const auto notifyFilter = static_cast<DWORD>(filter);

        trackingThread = std::jthread(
            [this, bTrackingRecursively, notifyFilter, ioCheckingPeriod](std::stop_token stopToken)
            {
                WCHAR fileNameBuffer[MAX_PATH]{ 0 };

                OVERLAPPED overlapped{ 0 };
                overlapped.hEvent = ioEventHandle;
                auto ioFuture = ioPromise.get_future();

                while (!stopToken.stop_requested())
                {
                    IG_CHECK(directoryHandle != INVALID_HANDLE_VALUE);
                    DWORD bytesReturned{ 0 };
                    const bool bRequestSucceeded = ReadDirectoryChangesW(directoryHandle,
                                                                         /* 주어진 버퍼 크기만큼 읽어온 다음, 덜 읽어왔으면 다음에 무조건 읽어 온다. */
                                                                         buffer.data(), ReservedBufferSizeInBytes,
                                                                         bTrackingRecursively, notifyFilter,
                                                                         &bytesReturned, &overlapped, 0);

                    if (bRequestSucceeded)
                    {
                        DWORD lastError = ERROR_IO_INCOMPLETE;
                        DWORD transferedBytes{ 0 };
                        while (!GetOverlappedResult(directoryHandle, &overlapped, &transferedBytes, FALSE))
                        {
                            lastError = GetLastError();
                            if (ioFuture.wait_for(ioCheckingPeriod) != std::future_status::timeout)
                            {
                                IG_CHECK(lastError == ERROR_IO_INCOMPLETE);
                                return;
                            }
                        }

                        const auto* notifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(buffer.data());
                        while (notifyInfo != nullptr && transferedBytes >= sizeof(FILE_NOTIFY_INFORMATION))
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
                            notificationQueue.push(std::move(newNotification));

                            if (notifyInfo->NextEntryOffset > 0)
                            {
                                notifyInfo = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(reinterpret_cast<const uint8_t*>(notifyInfo) + notifyInfo->NextEntryOffset);
                            }
                            else
                            {
                                notifyInfo = nullptr;
                            }
                        }
                    }
                    else
                    {
                        if (ioFuture.wait_for(ioCheckingPeriod) != std::future_status::timeout)
                        {
                            return;
                        }
                    }
                }
            });

        return EFileTrackerResult::Success;
    }

    void AsyncFileTracker::StopTracking()
    {
        if (IsTracking())
        {
            trackingThread.request_stop();
            ioPromise.set_value();
            CancelIo(directoryHandle);
            trackingThread.join();

            IG_CHECK(directoryHandle != INVALID_HANDLE_VALUE);
            CloseHandle(directoryHandle);
            directoryHandle = INVALID_HANDLE_VALUE;

            IG_CHECK(ioEventHandle);
            CloseHandle(ioEventHandle);
            ioEventHandle = INVALID_HANDLE_VALUE;

            notificationQueue.clear();
            path.clear();

            ioPromise = {};
        }
    }

    std::optional<FileNotification> AsyncFileTracker::TryGetNotification()
    {
        IG_CHECK(IsTracking());
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
        IG_CHECK(notification);
        return *notification;
    }

    const FileNotification& AsyncFileTracker::Iterator::operator*() const noexcept
    {
        IG_CHECK(notification);
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