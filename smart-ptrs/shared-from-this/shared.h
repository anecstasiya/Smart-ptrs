#pragma once

#include "sw_fwd.h"  // Forward declaration
#include <utility>
#include <cstddef>  // std::nullptr_t

// https://en.cppreference.com/w/cpp/memory/shared_ptr

struct ControlBlock {
public:
    virtual void OnZeroShared() = 0;
    virtual void* GetRaw() = 0;
    virtual const void* GetRaw() const = 0;

    ControlBlock() = default;
    ControlBlock(size_t shared, size_t weak) : shared_cnt_(shared), weak_cnt_(weak) {
    }

    void IncrShared() {
        shared_cnt_++;
    }

    void OnZeroWeak() {
        delete this;
    }

    void IncrWeak() {
        weak_cnt_++;
    }

    void DecrShared() {
        shared_cnt_--;
        if (shared_cnt_ == 0) {
            OnZeroShared();
            DecrWeak();
        }
    }

    void DecrWeak() {
        weak_cnt_--;
        if (weak_cnt_ == 0) {
            OnZeroWeak();
        }
    }

    size_t UseCnt() {
        return shared_cnt_;
    }
    size_t UseWeak() {
        return weak_cnt_;
    }
    virtual ~ControlBlock() {
    }

private:
    size_t shared_cnt_ = 0;
    size_t weak_cnt_ = 1;
};

template <typename T>
struct ControlBlockPtr : public ControlBlock {
public:
    ControlBlockPtr(T* ptr = nullptr) : ptr_(ptr) {
    }

    void OnZeroShared() override {
        delete ptr_;
    }

    virtual void* GetRaw() override {
        return ptr_;
    }

    virtual const void* GetRaw() const override {
        return ptr_;
    }

private:
    T* ptr_ = nullptr;
};

template <typename T>
struct ControlBlockObj : public ControlBlock {
public:
    std::aligned_storage_t<sizeof(T), alignof(T)> obj_;
    template <typename... Args>
    ControlBlockObj(Args&&... args) {
        new (&obj_) T(std::forward<Args>(args)...);
    }

    void OnZeroShared() override {
        reinterpret_cast<T*>(GetRaw())->~T();
    }

    virtual void* GetRaw() override {
        return &obj_;
    }

    virtual const void* GetRaw() const override {
        return &obj_;
    }
};
struct Dumm {};

template <typename T>
class EnableSharedFromThis {
    template <typename>
    friend class SharedPtr;

public:
    SharedPtr<T> SharedFromThis() {
        return weak_ptr_.Lock();
    }
    SharedPtr<const T> SharedFromThis() const {
        return weak_ptr_.Lock();
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_ptr_;
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_ptr_;
    }

protected:
    EnableSharedFromThis() noexcept = default;
    EnableSharedFromThis(const EnableSharedFromThis&) noexcept = default;
    EnableSharedFromThis& operator=(const EnableSharedFromThis&) noexcept = default;
    ~EnableSharedFromThis() = default;

private:
    void SetWeakPtr(ControlBlock* cnt, T* ptr) noexcept {
        weak_ptr_.Respect(cnt, ptr);
    }

    WeakPtr<T> weak_ptr_;
};

template <typename T>
class SharedPtr {
    template <class>
    friend class WeakPtr;
    template <class>
    friend class SharedPtr;
    template <typename>
    friend class EnabledSharedFromThis;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    template <typename U>
    void Override(ControlBlock* cnt, U* ptr) {
        if constexpr (requires(U* cont) { cont->SetWeakPtr(cnt, cont); }) {
            ptr->SetWeakPtr(cnt, ptr);
        }
    }
    SharedPtr() = default;
    SharedPtr(std::nullptr_t) {
        ptr_ = nullptr;
        cnt_ = nullptr;
    }
    SharedPtr(ControlBlock* cnt, T* ptr) : ptr_(ptr), cnt_(cnt) {
        if (cnt_ != nullptr) {
            // ptr_ = reinterpret_cast<T*>(cnt_->GetRaw());
            cnt_->IncrShared();
            // cnt_->IncrWeak();
        }
    }
    SharedPtr(ControlBlock* cnt, Dumm) : cnt_(cnt) {
        if (cnt_ != nullptr) {
            ptr_ = reinterpret_cast<T*>(cnt_->GetRaw());
            cnt_->IncrShared();
            // cnt_->IncrWeak();
            Override(cnt_, ptr_);
        }
    }
    template <typename U>
    explicit SharedPtr(U* ptr)
        requires(std::is_convertible_v<U*, T*>)
        : cnt_(new ControlBlockPtr<U>(ptr)) {
        cnt_->IncrShared();
        ptr_ = static_cast<T*>(cnt_->GetRaw());
        Override(cnt_, ptr);
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), cnt_(other.cnt_) {
        if (cnt_ != nullptr) {
            cnt_->IncrShared();
            // cnt_->IncrWeak();
        }
    }

    template <typename U>
    SharedPtr(const SharedPtr<U>& other) : ptr_(other.ptr_), cnt_(other.cnt_) {
        if (cnt_ != nullptr) {
            cnt_->IncrShared();
            // cnt_->IncrWeak();
        }
    }

    SharedPtr(SharedPtr&& other) noexcept {
        // if (&other != this) {
        cnt_ = std::move(other.cnt_);
        ptr_ = std::move(other.ptr_);
        other.ptr_ = nullptr;
        other.cnt_ = nullptr;
        // }
    }

    template <typename U>
    SharedPtr(SharedPtr<U>&& other) noexcept
        requires(std::is_convertible_v<U*, T*>)
    {
        // if (&other != this) {
        cnt_ = std::move(other.cnt_);
        ptr_ = std::move(static_cast<T*>(other.ptr_));
        other.ptr_ = nullptr;
        other.cnt_ = nullptr;
        // }
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) noexcept : ptr_(ptr), cnt_(other.cnt_) {
        if (cnt_ != nullptr) {
            cnt_->IncrShared();
            // cnt_->IncrWeak();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename U>
    SharedPtr(const WeakPtr<U>& other)
        requires(std::is_convertible_v<U*, T*>)
        : ptr_(other.ptr_), cnt_(other.cnt_) {
        if (cnt_ == nullptr || cnt_->UseCnt() == 0) {
            ptr_ = nullptr;
            cnt_ = nullptr;
            throw BadWeakPtr{};
        } else {
            cnt_->IncrShared();
        }
        // cnt_->IncrWeak();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (&other != this) {
            if (other.cnt_ != nullptr) {
                // other.cnt_->IncrWeak();
                other.cnt_->IncrShared();
            }
            if (cnt_ != nullptr) {
                cnt_->DecrShared();
                // cnt_->DecrWeak();
            }
            ptr_ = other.ptr_;
            cnt_ = other.cnt_;
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (&other != this) {
            if (cnt_ != nullptr) {
                cnt_->DecrShared();
                // cnt_->DecrWeak();
            }
            ptr_ = other.ptr_;
            cnt_ = other.cnt_;

            other.ptr_ = nullptr;
            other.cnt_ = nullptr;
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (cnt_ != nullptr) {
            cnt_->DecrShared();
            // cnt_->DecrWeak();

            cnt_ = nullptr;
            ptr_ = nullptr;
        }
    }
    template <typename U>
    void Reset(U* ptr)
        requires(std::is_convertible_v<U*, T*>)
    {
        ControlBlock* cnt1 = nullptr;
        U* ptr1 = nullptr;

        if (ptr != nullptr) {
            cnt1 = new ControlBlockPtr<U>(ptr);
            cnt1->IncrShared();
            // cnt1->IncrWeak();
            ptr1 = reinterpret_cast<U*>(cnt1->GetRaw());
        }
        if (cnt_ != nullptr) {
            cnt_->DecrShared();
            // cnt_->DecrWeak();
        }
        cnt_ = cnt1;
        ptr_ = ptr1;
        if (cnt_ != nullptr && ptr_ != nullptr) {
            Override(cnt_, ptr);
        }
    }

    void Reset(T* ptr) {
        ControlBlock* cnt1 = nullptr;
        T* ptr1 = nullptr;

        if (ptr != nullptr) {
            cnt1 = new ControlBlockPtr<T>(ptr);
            cnt1->IncrShared();
            // cnt1->IncrWeak();
            ptr1 = reinterpret_cast<T*>(cnt1->GetRaw());
        }
        if (cnt_ != nullptr) {
            cnt_->DecrShared();
            // cnt_->DecrWeak();
        }
        cnt_ = cnt1;
        ptr_ = ptr1;
        if (cnt_ != nullptr && ptr_ != nullptr) {
            Override(cnt_, ptr);
        }
    }

    template <typename U>
    void Swap(SharedPtr<U>& other) noexcept
        requires(std::is_same_v<T, U>)
    {
        std::swap(other.ptr_, ptr_);
        std::swap(cnt_, other.cnt_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }

    T& operator*() const {
        return *ptr_;
    }

    T* operator->() const {
        return ptr_;
    }

    size_t UseCount() const {
        if (cnt_ == nullptr) {
            return 0;
        } else {
            return cnt_->UseCnt();
        }
    }

    explicit operator bool() const {
        return (ptr_ != nullptr);
    }

private:
    T* ptr_ = nullptr;
    ControlBlock* cnt_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    ControlBlockObj<T>* cnt = new ControlBlockObj<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(cnt, Dumm{});
}

template <class U>
SharedPtr(const WeakPtr<U>&) -> SharedPtr<U>;

// Look for usage examples in tests
