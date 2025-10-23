#pragma once
#include <cstdint>

class Padding
{
public:
    static constexpr uint32_t DEFAULT_PADDING_ONE = 32;
    static constexpr uint32_t DEFAULT_PADDING_ONZ = 2048;

    template <typename T>
    static T Pad(T value, T padding)
    {
      //  if (value & (padding - 1)) 
            return (value + padding - 1) & ~(padding - 1);
        return value;
    }

    template <typename T>
    static T CalculateSizeForOne(T oneSize, T onzSize, T additional_padding) {
        T basePadding;
        T paddedOnz = CalculateSizeForOnz(onzSize);
        basePadding = (paddedOnz - onzSize);
        return Pad<T>(oneSize + basePadding + (additional_padding == 0 ? 0 : Pad<T>(additional_padding, DEFAULT_PADDING_ONE)) , DEFAULT_PADDING_ONE);
    }

    template <typename T>
    static T CalculateSizeForOnz(T onzSize) 
    {
        return Pad(onzSize, static_cast<T>(DEFAULT_PADDING_ONZ));
    }
};