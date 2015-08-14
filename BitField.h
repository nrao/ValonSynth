#ifndef BITFIELD_H
#define BITFIELD_H

// Credit to Evan Teran for the idea:
// http://blog.codef00.com/2014/12/06/portable-bitfields-using-c11/

#include <cstddef>
#include <cstdint>
#include <type_traits>

template <typename T, std::size_t Index, std::size_t Width>
class BitField {
    static_assert(std::is_integral<T>::value,
                  "BitField must use an integral type");

    static const T Max = (1U << Width) - 1U;
    static const T Mask = Max << Index;

public:
    BitField& operator=(T v) {
        value = (value & ~Mask) | ((v & Max) << Index);
        return *this;
    }

    operator T() const {
        return (value >> Index) & Max;
    }

#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || defined(__clang__)
    explicit
#endif
    operator bool() const {
        return value & Mask;
    }

    BitField& operator+=(T v) {
        value = (value + (v << Index)) & Mask;
        return *this;
    }

    BitField& operator-=(T v) {
        value = (value - (v << Index)) & Mask;
        return *this;
    }

    BitField& operator++() { return *this += 1; }
    BitField& operator--() { return *this -= 1; }

    T operator++(int) {
        T r = *this;
        ++*this;
        return r;
    }

    T operator--(int) {
        T r = *this;
        --*this;
        return r;
    }

private:
    T value;
};

#endif//BITFIELD_H
