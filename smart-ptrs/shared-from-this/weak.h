#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    template <class>
    friend class SharedPtr;
    template <class>
    friend class WeakPtr;
    template <typename>
    friend class EnabledSharedFromThis;
    WeakPtr() = default;

    WeakPtr(const WeakPtr& other) noexcept : ptr_(other.ptr_), cnt_(other.cnt_) {
        if (cnt_) {
            cnt_->IncrWeak();
        }
    }

    void Respect(ControlBlock* cnt, T* ptr) noexcept {
        if (cnt != nullptr) {
            if (ptr_ != ptr || cnt_ != cnt) {
                ptr_ = ptr;
                cnt_ = cnt;
                cnt->IncrWeak();
            }
        }
    }
    template <typename U>
    WeakPtr(const WeakPtr<U>& other) noexcept
        requires(std::is_convertible_v<U*, T*>)
        : ptr_(other.ptr_), cnt_(other.cnt_) {
        if (cnt_ != nullptr) {
            // cnt_->IncrShared();
            cnt_->IncrWeak();
        }
    }

    WeakPtr(WeakPtr&& other) noexcept {
        // if (&other != this) {
        cnt_ = std::move(other.cnt_);
        ptr_ = std::move(other.ptr_);
        other.ptr_ = nullptr;
        other.cnt_ = nullptr;
        // }
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    template <typename U>
    WeakPtr(const SharedPtr<U>& other) noexcept
        requires(std::is_convertible_v<U*, T*>)
        : ptr_(other.ptr_), cnt_(other.cnt_) {
        if (cnt_ != nullptr) {
            // cnt_->IncrShared();
            cnt_->IncrWeak();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (&other != this) {
            if (other.cnt_ != nullptr) {
                other.cnt_->IncrWeak();
                // other.cnt_->IncrShared();
            }
            if (cnt_ != nullptr) {
                // cnt_->DecrShared();
                cnt_->DecrWeak();
            }
            ptr_ = other.ptr_;
            cnt_ = other.cnt_;
        }
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) {
        if (&other != this) {
            if (cnt_ != nullptr) {
                // cnt_->DecrShared();
                cnt_->DecrWeak();
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

    ~WeakPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (cnt_ != nullptr) {
            // cnt_->DecrShared();
            cnt_->DecrWeak();

            cnt_ = nullptr;
            ptr_ = nullptr;
        }
    }

    template <typename U>
    void Swap(WeakPtr<U>& other) noexcept
        requires(std::is_same_v<T, U>)
    {
        std::swap(other.ptr_, ptr_);
        std::swap(cnt_, other.cnt_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (cnt_ == nullptr) {
            return 0;
        } else {
            return cnt_->UseCnt();
        }
    }
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T> Lock() const {
        if (cnt_ == nullptr || cnt_->UseCnt() == 0) {
            return {};
        }
        // cnt_->IncrShared();
        return SharedPtr<T>(cnt_, ptr_);
    }

private:
    T* ptr_ = nullptr;
    ControlBlock* cnt_ = nullptr;
};
