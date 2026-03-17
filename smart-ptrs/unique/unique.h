#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <utility>
#include <type_traits>

template <typename T>
struct Slug;

template <typename T>
struct Slug {
    void operator()(T* ptr) {
        if (ptr != nullptr) {
            delete ptr;
        }
    }
    Slug() = default;
    template <typename F>
    Slug(F&&) {
    }
};
template <class T>
struct Slug<T[]> {
    void operator()(T* ptr) {
        if (ptr != nullptr) {
            delete[] ptr;
        }
    }
    Slug() = default;
    template <typename F>
    Slug(T&&) {
    }
};
// Primary template
template <typename T, typename Deleter = Slug<T> >
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr() : ptr_(nullptr, Deleter{}) {
    }
    explicit UniquePtr(T* p) noexcept {
        Reset(p);
    }

    UniquePtr(T* ptr, Deleter deleter) noexcept {
        Reset(ptr);
        ptr_.GetSecond() = std::move(deleter);
    }

    UniquePtr(UniquePtr&& other) noexcept {
        if (&other != this) {
            Reset(other.Release());
            ptr_.GetSecond() = std::move(other.GetDeleter());
        }
    }
    template <typename F, typename S>
    UniquePtr(UniquePtr<F, S>&& other) noexcept {
        // if(&other != this) {
        Reset(other.Release());
        ptr_.GetSecond() = std::move(other.GetDeleter());
        // }
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (&other != this) {
            Reset(other.Release());
            ptr_.GetSecond() = std::move(other.GetDeleter());
        }
        return *this;
    }
    template <typename F, typename S>
    UniquePtr& operator=(UniquePtr<F, S>&& other) noexcept {
        // if(&other != this) {
        Reset(other.Release());
        ptr_.GetSecond() = std::move(other.GetDeleter());
        // }
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* was = ptr_.GetFirst();
        ptr_.GetFirst() = nullptr;
        return was;
    }

    void Reset(T* ptr = nullptr) {
        T* old = ptr_.GetFirst();
        if (ptr != old) {

            ptr_.GetFirst() = ptr;
            if (old != nullptr) {
                GetDeleter()(old);
            }
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_.GetSecond(), other.ptr_.GetSecond());
        std::swap(ptr_.GetFirst(), other.ptr_.GetFirst());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_.GetFirst();
    };

    Deleter& GetDeleter() {
        return ptr_.GetSecond();
    }

    const Deleter& GetDeleter() const {
        return ptr_.GetSecond();
    };

    explicit operator bool() const {
        return ptr_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    template <typename F = T>
    F& operator*() const
        requires(!std::is_void_v<F>)
    {
        return *ptr_.GetFirst();
    }
    template <typename F = T>
    F* operator->() const
        requires(!std::is_void_v<F>)
    {
        return ptr_.GetFirst();
    }

private:
    CompressedPair<T*, Deleter> ptr_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr() = default;
    explicit UniquePtr(T* p) noexcept {
        Reset();
        ptr_.GetFirst() = p;
    }

    UniquePtr(T* ptr, Deleter deleter) noexcept {
        Reset(ptr);
        ptr_.GetSecond() = std::move(deleter);
    }

    template <typename F, typename S>
    UniquePtr(UniquePtr<F, S>&& other) noexcept {
        // if(&other != this) {
        Reset(other.Release());
        ptr_.GetSecond() = std::move(other.GetDeleter());
        // }
    }

    UniquePtr(UniquePtr&& other) noexcept {
        if (&other != this) {
            Reset(other.Release());
            ptr_.GetSecond() = std::move(other.GetDeleter());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (&other != this) {
            Reset(other.Release());
            ptr_.GetSecond() = std::move(other.GetDeleter());
        }
        return *this;
    }
    template <typename F, typename S>
    UniquePtr& operator=(UniquePtr<F, S>&& other) noexcept {
        // if(&other != this) {
        Reset(other.Release());
        ptr_.GetSecond() = std::move(other.GetDeleter());
        // }
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* was = ptr_.GetFirst();
        ptr_.GetFirst() = nullptr;
        return was;
    }

    void Reset(T* ptr = nullptr) {
        T* old = ptr_.GetFirst();
        if (ptr != old) {
            ptr_.GetFirst() = ptr;

            if (old != nullptr) {
                GetDeleter()(old);
            }
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(ptr_.GetSecond(), other.ptr_.GetSecond());
        std::swap(ptr_.GetFirst(), other.ptr_.GetFirst());
    }
    T* Get() const {
        return ptr_.GetFirst();
    };

    Deleter& GetDeleter() {
        return ptr_.GetSecond();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    const Deleter& GetDeleter() const {
        return ptr_.GetSecond();
    };

    explicit operator bool() const {
        return ptr_.GetFirst() != nullptr;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator[](int ind) {
        return ptr_.GetFirst()[ind];
    }

    const T& operator[](int ind) const {
        return ptr_.GetFirst()[ind];
    }

private:
    CompressedPair<T*, Deleter> ptr_;
};
