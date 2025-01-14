#pragma once
#include "Igniter/Igniter.h"
#include "Igniter/Core/String.h"

namespace ig::details
{
    class FileWatchTask
    {
      public:
        struct promise_type
        {
            FileWatchTask get_return_object() { return FileWatchTask{std::coroutine_handle<promise_type>::from_promise(*this)}; }

            auto initial_suspend() { return std::suspend_never{}; }

            void return_void() {}

            auto final_suspend() noexcept { return std::suspend_never{}; }
            auto unhandled_exception() { throw; }
        };

      public:
        FileWatchTask(std::coroutine_handle<promise_type> handle)
            : handle(handle)
        {
        }

        FileWatchTask(const FileWatchTask&) = delete;
        FileWatchTask(FileWatchTask&&) = delete;

        ~FileWatchTask()
        {
            if (static_cast<bool>(handle))
            {
                handle.destroy();
            }
        }

        FileWatchTask& operator=(const FileWatchTask&) = delete;
        FileWatchTask& operator=(FileWatchTask&&) = delete;

        void Resume()
        {
            if (!handle.done())
            {
                handle.resume();
            }
        }

        bool IsValid() const { return handle && !handle.done(); }

      private:
        std::coroutine_handle<promise_type> handle;
    };
} // namespace ig::details

namespace ig
{
    enum class EFileWatchFilterFlags
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

    IG_ENUM_FLAGS(EFileWatchFilterFlags);

    enum class EFileWatchAction
    {
        Added = FILE_ACTION_ADDED,                     /* 파일이 디렉터리에 추가됨 */
        Removed = FILE_ACTION_REMOVED,                 /* 파일이 디렉터리에서 삭제됨 */
        Modified = FILE_ACTION_MODIFIED,               /* 파일이 수정 됨 */
        RenamedOldName = FILE_ACTION_RENAMED_OLD_NAME, /* 이름이 변경된 파일의 변경 전 이름 */
        RenamedNewName = FILE_ACTION_RENAMED_NEW_NAME  /* 이름이 변경된 파일의 변경 후 이름 */
    };

    struct [[nodiscard]] FileChangeInfo
    {
      public:
        EFileWatchAction Action;
        Path Path{};
        uint64_t CreationTime{};
        uint64_t LastModificationTime{};
        uint64_t LastChangeTime{};
        uint64_t LastAccessTime{};
        uint64_t FileSize{};
    };

    class CoFileWatcher final
    {
      public:
        CoFileWatcher(const String directoryPathStr, const EFileWatchFilterFlags filters, const bool bWatchRecursively = true);
        ~CoFileWatcher();

        std::vector<FileChangeInfo> RequestChanges(const bool bEnsureCatch, const bool bIgnoreDirectory = true);

      private:
        bool IsReadyToWatch() const { return directory != INVALID_HANDLE_VALUE; }

        static details::FileWatchTask Watch(CoFileWatcher* watcher);

      private:
        Path directoryPath;
        HANDLE directory{INVALID_HANDLE_VALUE};

        bool bStopWatching{false};
        bool bWatchRecursively = true;
        EFileWatchFilterFlags filters = EFileWatchFilterFlags::ChangeFileName | EFileWatchFilterFlags::ChangeLastWrite;
        std::vector<FileChangeInfo> buffer{};

        bool bEnsureCatchChanges = false;
        bool bIgnoreDirectoryChanges = true;

        details::FileWatchTask task;
    };
} // namespace ig
