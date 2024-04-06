#pragma once
#include <Igniter.h>
#include <Core/Engine.h>
#include <Core/HashUtils.h>
#include <Core/DeferredDeallocator.h>

namespace ig
{
    template <typename T>
    class RefHandle;
    template <typename T, typename Destroyer>
    class Handle;

    class HandleManager;
    namespace details
    {
        class HandleImpl final
        {
            template <typename T>
            friend class RefHandle;

            template <typename T, typename Destroyer>
            friend class Handle;

        public:
            HandleImpl(const HandleImpl& other) = default;
            HandleImpl(HandleImpl&& other) noexcept;
            ~HandleImpl() = default;

            HandleImpl& operator=(const HandleImpl& other) = default;
            HandleImpl& operator=(HandleImpl&& other) noexcept;

            uint8_t* GetAddressOf(const uint64_t typeHashValue);
            const uint8_t* GetAddressOf(const uint64_t typeHashValue) const;

            uint8_t* GetVaildAddressOf(const uint64_t typeHashValue);
            const uint8_t* GetVaildAddressOf(const uint64_t typeHashValue) const;

            bool IsValid() const { return owner != nullptr && handle != InvalidHandle; }
            bool IsAlive(const uint64_t typeHashValue) const;
            bool IsPendingDeallocation(const uint64_t typeHashValue) const;

            void Deallocate(const uint64_t typeHashValue);
            void MarkAsPendingDeallocation(const uint64_t typeHashVal);

            size_t GetHash() const { return handle; }

        private:
            HandleImpl() = default;
            HandleImpl(HandleManager& handleManager, const uint64_t typeHashVal, const size_t sizeOfType, const size_t alignOfType);

        public:
            static constexpr uint64_t InvalidHandle = 0xffffffffffffffffUi64;

        private:
            HandleManager* owner{ nullptr };
            uint64_t handle{ InvalidHandle };
        };
    } // namespace details

    template <typename T>
    class RefHandle
    {
    public:
        RefHandle() = default;

        explicit RefHandle(const details::HandleImpl newHandle) : handle(newHandle)
        {
        }

        RefHandle(const RefHandle& other) = default;
        RefHandle(RefHandle&& other) noexcept = default;
        virtual ~RefHandle() = default;

        RefHandle& operator=(const RefHandle& other) = default;
        RefHandle& operator=(RefHandle&& other) noexcept = default;

        [[nodiscard]] T& operator*()
        {
            IG_CHECK(IsAlive());
            T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return *instancePtr;
        }

        [[nodiscard]] const T& operator*() const
        {
            IG_CHECK(IsAlive());
            T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return *instancePtr;
        }

        [[nodiscard]] T* operator->()
        {
            IG_CHECK(IsAlive());
            T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return instancePtr;
        }

        [[nodiscard]] const T* operator->() const
        {
            IG_CHECK(IsAlive());
            T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return instancePtr;
        }

        [[nodiscard]] bool IsAlive() const { return handle.IsValid() && handle.IsAlive(EvaluatedTypeHashVal); }
        [[nodiscard]] operator bool() const { return IsAlive(); }

        [[nodiscard]] size_t GetHash() const { return handle.GetHash(); }

    protected:
        details::HandleImpl GetHandle() { return handle; }

    public:
        static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

    protected:
        details::HandleImpl handle{};
    };

    template <typename T>
    class DefaultFinalizer final
    {
    public:
        void operator()(T*)
        {
            /* Empty */
            // DO NOT DESTROY INSTANCE IN FINALIZER -> It caused of access after delete.
        }
    };

    template <typename T>
    using FuncPtrFinalizer = void (*)(T*);

    template <typename T, typename Finalizer = DefaultFinalizer<T>>
    class UniqueRefHandle final : public RefHandle<T>
    {
    public:
        UniqueRefHandle() = default;

        explicit UniqueRefHandle(const details::HandleImpl newHandle) : RefHandle<T>(newHandle)
        {
            if constexpr (std::is_pointer_v<Finalizer>)
            {
                finalizer = nullptr;
            }
        }

        template <typename Fn>
            requires(std::is_same_v<std::decay_t<Fn>, Finalizer>)
        explicit UniqueRefHandle(const details::HandleImpl newHandle, Fn&& finalizer) : finalizer(std::forward<Fn>(finalizer)),
                                                                                        RefHandle<T>(newHandle)
        {
        }

        UniqueRefHandle(const UniqueRefHandle& other) = delete;
        UniqueRefHandle(UniqueRefHandle&& other) noexcept : RefHandle<T>(std::move(other.handle))
        {
            if constexpr (std::is_pointer_v<Finalizer>)
            {
                this->finalizer = std::exchange(other.finalizer, nullptr);
            }
            else
            {
                this->finalizer = std::move(other.finalizer);
            }
        }

        ~UniqueRefHandle()
        {
            if (RefHandle<T>::IsAlive())
            {
                details::HandleImpl handle = RefHandle<T>::GetHandle();
                if constexpr (std::is_pointer_v<Finalizer>)
                {
                    if (finalizer != nullptr)
                    {
                        (*finalizer)(reinterpret_cast<T*>(handle.GetAddressOf(RefHandle<T>::EvaluatedTypeHashVal)));
                    }
                }
                else
                {
                    finalizer(reinterpret_cast<T*>(handle.GetAddressOf(RefHandle<T>::EvaluatedTypeHashVal)));
                }
            }
        }

        UniqueRefHandle& operator=(const UniqueRefHandle&) = delete;
        UniqueRefHandle& operator=(UniqueRefHandle&& other) noexcept
        {
            RefHandle<T>::operator=(other);
            if constexpr (std::is_pointer_v<Finalizer>)
            {
                this->finalizer = std::exchange(other.finalizer, nullptr);
            }
            else
            {
                this->finalizer = std::move(other.finalizer);
            }

            return *this;
        }

        [[nodiscard]] RefHandle<const T> MakeRef() const
        {
            return RefHandle<const T>{ RefHandle<T>::handle };
        }

        [[nodiscard]] RefHandle<T> MakeRef()
        {
            return RefHandle<T>{ RefHandle<T>::handle };
        }

    private:
        [[no_unique_address]] Finalizer finalizer{};
    };

    template <typename T>
    class DefaultDestroyer final
    {
    public:
        void operator()(details::HandleImpl handle, const uint64_t typeHashVal, T* instance) const
        {
            IG_CHECK(instance != nullptr);
            if (handle.IsValid() && typeHashVal != InvalidHashVal)
            {
                if (instance != nullptr)
                {
                    instance->~T();
                }
                handle.Deallocate(typeHashVal);
            }
        }
    };

    template <typename T>
    class DeferredDestroyer final
    {
    public:
        void operator()(details::HandleImpl handle, const uint64_t typeHashVal, T* instance) const
        {
            auto& deferredDeallocator = Igniter::GetDeferredDeallocator();
            deferredDeallocator.RequestDeallocation(
                [handle, typeHashVal, instance]()
                {
                    details::HandleImpl targetHandle = handle;
                    IG_CHECK(instance != nullptr);
                    if (targetHandle.IsValid() && typeHashVal != InvalidHashVal)
                    {
                        if (instance != nullptr)
                        {
                            instance->~T();
                        }

                        targetHandle.Deallocate(typeHashVal);
                    }
                });
        }
    };

    template <typename T>
    using FuncPtrDestroyer = void (*)(details::HandleImpl, const uint64_t, T*);

    template <typename T, typename Destroyer = DefaultDestroyer<T>>
    class Handle final
    {
    public:
        Handle() = default;
        Handle(const Handle&) = delete;

        Handle(Handle&& other) noexcept
            : handle(std::move(other.handle)),
              destroyer(std::move(other.destroyer))
        {
            if constexpr (std::is_pointer_v<Destroyer>)
            {
                other.destroyer = nullptr;
            }
        }

        template <typename Dx>
        Handle(Dx&& destroyer)
            : destroyer(std::forward<Dx>(destroyer))
        {
        }

        template <typename... Args, typename Dx>
            requires(std::is_same_v<std::decay_t<Dx>, Destroyer>)
        Handle(HandleManager& handleManager, Dx&& destroyer, Args&&... args)
            : handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T)),
              destroyer(std::forward<Dx>(destroyer))
        {
            IG_CHECK(IsAlive());
            if constexpr (std::is_pointer_v<Destroyer>)
            {
                IG_CHECK(destroyer != nullptr);
            }

            T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            new (instancePtr) T(std::forward<Args>(args)...);
        }

        template <typename... Args>
            requires(!std::is_pointer_v<Destroyer> && std::is_constructible_v<Destroyer>)
        Handle(HandleManager& handleManager, Args&&... args)
            : handle(handleManager, EvaluatedTypeHashVal, sizeof(T), alignof(T))
        {
            IG_CHECK(IsAlive());
            T* instancePtr = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            new (instancePtr) T(std::forward<Args>(args)...);
        }

        ~Handle()
        {
            Destroy();
        }

        Handle& operator=(const Handle&) = delete;
        Handle& operator=(Handle&& other) noexcept
        {
            Destroy();
            handle = std::move(other.handle);
            if constexpr (std::is_pointer_v<Destroyer>)
            {
                destroyer = std::exchange(other.destroyer, nullptr);
            }
            else
            {
                destroyer = std::move(other.destroyer);
            }
            return *this;
        }

        [[nodiscard]] T& operator*()
        {
            T* instancePtr = reinterpret_cast<T*>(handle.GetVaildAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return *instancePtr;
        }

        [[nodiscard]] const T& operator*() const
        {
            const T* instancePtr = reinterpret_cast<const T*>(handle.GetVaildAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return *instancePtr;
        }

        [[nodiscard]] T* operator->()
        {
            T* instancePtr = reinterpret_cast<T*>(handle.GetVaildAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return instancePtr;
        }

        [[nodiscard]] const T* operator->() const
        {
            const T* instancePtr = reinterpret_cast<const T*>(handle.GetVaildAddressOf(EvaluatedTypeHashVal));
            IG_CHECK(instancePtr != nullptr);
            return instancePtr;
        }

        [[nodiscard]] bool IsAlive() const { return handle.IsAlive(EvaluatedTypeHashVal); }
        [[nodiscard]] operator bool() const { return IsAlive(); }

        void Destroy()
        {
            if (IsAlive())
            {
                handle.MarkAsPendingDeallocation(EvaluatedTypeHashVal);
                T* instance = reinterpret_cast<T*>(handle.GetAddressOf(EvaluatedTypeHashVal));
                IG_CHECK(instance != nullptr);
                if constexpr (std::is_pointer_v<Destroyer>)
                {
                    IG_CHECK(destroyer != nullptr);
                    (*destroyer)(handle, EvaluatedTypeHashVal, instance);
                }
                else
                {
                    destroyer(handle, EvaluatedTypeHashVal, instance);
                }
                handle = {};
            }
        }

        [[nodiscard]] RefHandle<const T> MakeRef() const
        {
            return RefHandle<const T>{ handle };
        }

        [[nodiscard]] RefHandle<T> MakeRef()
        {
            return RefHandle<T>{ handle };
        }

        template <typename Finalizer>
        [[nodiscard]] UniqueRefHandle<const T, Finalizer> MakeUniqueRef(Finalizer&& finalizer) const
        {
            return UniqueRefHandle<const T, Finalizer>{ handle, finalizer };
        }

        template <typename Finalizer>
        [[nodiscard]] UniqueRefHandle<T, Finalizer> MakeUniqueRef(Finalizer&& finalizer)
        {
            return UniqueRefHandle<T, Finalizer>{ handle, finalizer };
        }

        [[nodiscard]] size_t GetHash() const { return handle.GetHash(); }

    public:
        static constexpr uint64_t EvaluatedTypeHashVal = HashOfType<T>;

    private:
        details::HandleImpl handle{};
        [[no_unique_address]] Destroyer destroyer{};
    };

    template <typename T>
    using DeferredHandle = Handle<T, DeferredDestroyer<T>>;
} // namespace ig

namespace std
{
    template <typename T, typename Dx>
    class hash<ig::Handle<T, Dx>> final
    {
    public:
        size_t operator()(const ig::Handle<T, Dx>& handle) const { return handle.GetHash(); }
    };

    template <typename T>
    class hash<ig::RefHandle<T>> final
    {
    public:
        size_t operator()(const ig::RefHandle<T>& handle) const { return handle.GetHash(); }
    };
} // namespace std