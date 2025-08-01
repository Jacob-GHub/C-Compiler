// LUT.h
#ifndef LUT_H
#define LUT_H
#include <cstdint>
#include <string_view>
using u8 = uint8_t;

struct LUT
{
    consteval LUT(std::string_view sv, bool null_term = false) : lut{}
    {
        for (u8 ch : sv)
        {
            lut[ch] = 1;
        }
        if (null_term)
        {
            lut[0] = 1;
        }
    }

    int operator()(u8 i) const { return lut[i]; }

private:
    u8 lut[256];
};

#endif