#pragma once
#include "Vector.h"

//Floating point to integer range compression. 
template<typename IntegerType, typename FloatType>
class IntegerRangeCompressor {
public:

    static_assert(std::is_integral_v<IntegerType>);
    static_assert(std::is_floating_point_v<FloatType>);

    static void getCompressionParameters(FloatType f_min, FloatType f_max, FloatType& scale, FloatType& bias) noexcept {
        constexpr auto i_min = static_cast<FloatType>(std::numeric_limits<IntegerType>::min() + 1);//Plus 1 is needed because of bug in IOI's code.
        constexpr auto i_max = static_cast<FloatType>(std::numeric_limits<IntegerType>::max());
        scale = -std::numeric_limits<IntegerType>::max() * (-f_max + f_min) / (i_max - i_min);
        bias = -((-f_min * i_max) + (f_max * i_min)) / (i_max - i_min);
    }

    //template<unsigned int VecSize>
    //static void getCompressionParameters(const Vec<FloatType, VecSize>& f_min, const Vec<FloatType, VecSize>& f_max, Vec<FloatType, VecSize>& scale, Vec<FloatType, VecSize>& bias) noexcept {
    //    for (int i = 0; i < VecSize; ++i)
    //        getCompressionParameters(f_min[i], f_max[i], scale[i], bias[i]);
    //}

    static inline FloatType decompress(IntegerType integer, FloatType scale, FloatType bias) noexcept {
        return (static_cast<FloatType>(integer) * scale) / std::numeric_limits<IntegerType>::max() + bias;
    }

    static inline IntegerType compress(FloatType fl, FloatType scale, FloatType bias) noexcept {
        return static_cast<IntegerType>(std::roundf(std::numeric_limits<IntegerType>::max() * (fl - bias) / scale));
    }

};