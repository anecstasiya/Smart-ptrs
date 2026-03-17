#pragma once

#include <utility>
#include <type_traits>

// Me think, why waste time write lot code, when few code do trick.
template <typename T>
constexpr bool kIsEmpty = std::is_empty_v<T> && !std::is_final_v<T>;

template <typename F, typename S, bool f = kIsEmpty<F>, bool s = kIsEmpty<S>>
class CompressedPair;

template <typename F, typename S>
class CompressedPair<F, S, false, false> {
public:
    CompressedPair() : first_(), second_() {
    }
    CompressedPair(const F& first, const S& second) : first_(first), second_(second) {
    }

    CompressedPair(F&& first, const S& second) : first_(std::move(first)), second_(second) {
    }

    CompressedPair(F&& first, S&& second) : first_(std::move(first)), second_(std::move(second)) {
    }

    CompressedPair(const F& first, S&& second) : first_(first), second_(std::move(second)) {
    }

    F& GetFirst() {
        return first_;
    }
    const F& GetFirst() const {
        return first_;
    }
    S& GetSecond() {
        return second_;
    }
    const S& GetSecond() const {
        return second_;
    };

private:
    F first_;
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, true, false> : private F {
public:
    CompressedPair() : F(), second_() {
    }
    CompressedPair(const F& first, const S& second) : F(first), second_(second) {
    }

    CompressedPair(F&& first, const S& second) : F(std::move(first)), second_(second) {
    }

    CompressedPair(F&& first, S&& second) : F(std::move(first)), second_(std::move(second)) {
    }

    CompressedPair(const F& first, S&& second) : F(first), second_(std::move(second)) {
    }

    F& GetFirst() {
        return static_cast<F&>(*this);
    }
    const F& GetFirst() const {
        return static_cast<const F&>(*this);
    }
    S& GetSecond() {
        return second_;
    }
    const S& GetSecond() const {
        return second_;
    };

private:
    S second_;
};

template <typename F, typename S>
class CompressedPair<F, S, false, true> : private S {
public:
    CompressedPair() : S(), first_() {
    }
    CompressedPair(const F& first, const S& second) : S(second), first_(first) {
    }

    CompressedPair(F&& first, const S& second) : S(second), first_(std::move(first)) {
    }

    CompressedPair(F&& first, S&& second) : S(std::move(second)), first_(std::move(first)) {
    }

    CompressedPair(const F& first, S&& second) : S(std::move(second)), first_(first) {
    }

    F& GetFirst() {
        return first_;
    }
    const F& GetFirst() const {
        return first_;
    }
    S& GetSecond() {
        return static_cast<S&>(*this);
    }
    const S& GetSecond() const {
        return static_cast<const S&>(*this);
    };

private:
    F first_;
};

template <typename T, bool flag>
struct CopyForBase : private T {
    CopyForBase() {
    }
    CopyForBase(const T& x) : T(x) {
    }
    CopyForBase(T&& x) : T(std::move(x)) {
    }

    T& Get() {
        return static_cast<T&>(*this);
    }
    const T& Get() const {
        return static_cast<const T&>(*this);
    }
};

template <typename F, typename S>
class CompressedPair<F, S, true, true> : private CopyForBase<F, true>,
                                         private CopyForBase<S, false> {
    using F1 = CopyForBase<F, true>;
    using S1 = CopyForBase<S, false>;

public:
    CompressedPair() {
    }
    CompressedPair(const F& first, const S& second) : F1(first), S1(second) {
    }

    CompressedPair(F&& first, const S& second) : F1(std::move(first)), S1(second) {
    }

    CompressedPair(F&& first, S&& second) : F1(std::move(first)), S1(std::move(second)) {
    }

    CompressedPair(const F& first, S&& second) : F1(first), S1(std::move(second)) {
    }

    F& GetFirst() {
        return static_cast<F1&>(*this).Get();
    }
    const F& GetFirst() const {
        return static_cast<const F1&>(*this).Get();
    }
    S& GetSecond() {
        return static_cast<S1&>(*this).Get();
    }
    const S& GetSecond() const {
        return static_cast<const S1&>(*this).Get();
    };

private:
};
