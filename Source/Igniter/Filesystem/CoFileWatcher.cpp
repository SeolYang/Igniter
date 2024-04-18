#include <Igniter.h>
#include <Filesystem/CoFileWatcher.h>
#include <Core/Log.h>

IG_DEFINE_LOG_CATEGORY(CoFileWatcher)

namespace ig
{
    CoFileWatcher::CoFileWatcher(const String directoryPathStr, const EFileWatchFilterFlags filters,
                                 const bool   bWatchRecursively /*= true*/)
        : directoryPath(directoryPathStr.ToStringView())
        , directory(CreateFile(directoryPath.c_str(),
                               GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                               nullptr))
        , filters(filters)
        , bWatchRecursively(bWatchRecursively)
        , task(CoFileWatcher::Watch(this))
    {
    }

    CoFileWatcher::~CoFileWatcher()
    {
        if (directory != INVALID_HANDLE_VALUE)
        {
            CloseHandle(directory);
        }
    }

    std::vector<FileChangeInfo> CoFileWatcher::RequestChanges(const bool bEnsureCatch, const bool bIgnoreDirectory)
    {
        IG_CHECK(IsReadyToWatch() && task.IsValid());
        this->bEnsureCatchChanges     = bEnsureCatch;
        this->bIgnoreDirectoryChanges = bIgnoreDirectory;
        task.Resume();
        std::vector<FileChangeInfo> tempBuffer{std::move(buffer)};
        return tempBuffer;
    }

    details::FileWatchTask CoFileWatcher::Watch(CoFileWatcher* watcher)
    {
        IG_CHECK(watcher != nullptr);
        IG_CHECK(watcher->IsReadyToWatch());

        constexpr size_t     ReservedRawBufferSizeInBytes = 1024Ui64 * 1024Ui64;
        std::vector<uint8_t> rawBuffer(ReservedRawBufferSizeInBytes);
        WCHAR                fileNameBuffer[MAX_PATH]{0};

        while (!watcher->bStopWatching)
        {
            OVERLAPPED overlapped{0};
            overlapped.hEvent = CreateEvent(nullptr, 0, 0, nullptr);

            const bool bRequestSucceeded = ReadDirectoryChangesExW(watcher->directory,
                                                                   rawBuffer.data(), ReservedRawBufferSizeInBytes,
                                                                   watcher->bWatchRecursively,
                                                                   static_cast<DWORD>(watcher->filters),
                                                                   nullptr, &overlapped, nullptr,
                                                                   ReadDirectoryNotifyExtendedInformation);

            if (bRequestSucceeded)
            {
                co_await std::suspend_always{};
                DWORD waitResult{};
                while (!watcher->bStopWatching)
                {
                    waitResult = WaitForSingleObject(overlapped.hEvent, watcher->bEnsureCatchChanges ? INFINITE : 0);
                    if (waitResult != WAIT_OBJECT_0)
                    {
                        co_await std::suspend_always{};
                    }
                    else
                    {
                        break;
                    }
                }

                if (waitResult == WAIT_OBJECT_0)
                {
                    DWORD transferedBytes{0};
                    GetOverlappedResult(watcher->directory, &overlapped, &transferedBytes, FALSE);
                    if (transferedBytes == 0)
                    {
                        break;
                    }

                    const auto* notifyInfo = reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(rawBuffer.
                        data());
                    while (notifyInfo != nullptr)
                    {
                        const bool bSuccessCopyFileName = SUCCEEDED(
                            StringCbCopyNW(fileNameBuffer, sizeof(fileNameBuffer), notifyInfo->FileName, notifyInfo->
                                FileNameLength));
                        if (!IG_ENSURE_MSG(bSuccessCopyFileName, "Failed to copy file name to buffer."))
                        {
                            continue;
                        }

                        const Path notifiedPath = watcher->directoryPath / fileNameBuffer;
                        if (!fs::is_directory(notifiedPath) || !watcher->bIgnoreDirectoryChanges)
                        {
                            watcher->buffer.emplace_back(FileChangeInfo{
                                static_cast<EFileWatchAction>(notifyInfo->Action),
                                watcher->directoryPath / fileNameBuffer,
                                static_cast<uint64_t>(notifyInfo->CreationTime.QuadPart),
                                static_cast<uint64_t>(notifyInfo->LastModificationTime.QuadPart),
                                static_cast<uint64_t>(notifyInfo->LastChangeTime.QuadPart),
                                static_cast<uint64_t>(notifyInfo->LastAccessTime.QuadPart),
                                static_cast<uint64_t>(notifyInfo->FileSize.QuadPart)
                            });
                        }

                        if (notifyInfo->NextEntryOffset > 0)
                        {
                            notifyInfo = reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(reinterpret_cast<
                                const uint8_t*>(notifyInfo) + notifyInfo->NextEntryOffset);
                        }
                        else
                        {
                            notifyInfo = nullptr;
                        }
                    }
                }

                CancelIo(overlapped.hEvent);
            }
            else
            {
                IG_LOG(CoFileWatcher, Warning, "Failed to request read directory changes.");
            }

            CloseHandle(overlapped.hEvent);
        }
    }
} // namespace ig
