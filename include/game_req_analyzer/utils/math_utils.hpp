#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <cmath>
#include <numbers>

namespace game_req {

namespace math {

constexpr f32 PI = std::numbers::pi_v<f32>;
constexpr f64 PI_D = std::numbers::pi_v<f64>;
constexpr f32 TAU = 2.0f * PI;
constexpr f64 TAU_D = 2.0 * PI_D;
constexpr f32 DEG2RAD = PI / 180.0f;
constexpr f32 RAD2DEG = 180.0f / PI;

template<Arithmetic T>
[[nodiscard]] constexpr T clamp(T value, T min, T max) noexcept {
    return value < min ? min : (value > max ? max : value);
}

template<Arithmetic T>
[[nodiscard]] constexpr T lerp(T a, T b, f32 t) noexcept {
    return a + static_cast<T>((b - a) * clamp(t, 0.0f, 1.0f));
}

template<Arithmetic T>
[[nodiscard]] constexpr T smoothstep(T edge0, T edge1, T x) noexcept {
    T t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
    return t * t * (T(3) - T(2) * t);
}

template<Arithmetic T>
[[nodiscard]] constexpr T smootherstep(T edge0, T edge1, T x) noexcept {
    T t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
    return t * t * t * (t * (t * 6 - 15) + 10);
}

[[nodiscard]] inline f32 sqrt_f32(f32 x) noexcept { return std::sqrt(x); }
[[nodiscard]] inline f64 sqrt_f64(f64 x) noexcept { return std::sqrt(x); }
[[nodiscard]] inline f32 rsqrt_f32(f32 x) noexcept { return 1.0f / std::sqrt(x); }
[[nodiscard]] inline f32 rcp_f32(f32 x) noexcept { return 1.0f / x; }

template<Arithmetic T>
[[nodiscard]] constexpr T min(T a, T b) noexcept { return a < b ? a : b; }
template<Arithmetic T>
[[nodiscard]] constexpr T max(T a, T b) noexcept { return a > b ? a : b; }

[[nodiscard]] inline f32 deg2rad(f32 deg) noexcept { return deg * DEG2RAD; }
[[nodiscard]] inline f32 rad2deg(f32 rad) noexcept { return rad * RAD2DEG; }

template<Arithmetic T>
[[nodiscard]] constexpr T abs(T x) noexcept { return x < 0 ? -x : x; }

template<Arithmetic T>
[[nodiscard]] constexpr T sign(T x) noexcept { return (x > 0) - (x < 0); }

template<Arithmetic T>
[[nodiscard]] constexpr T floor_div(T a, T b) noexcept {
    T q = a / b;
    T r = a % b;
    return (r != 0 && ((r < 0) != (b < 0))) ? q - 1 : q;
}

template<Arithmetic T>
[[nodiscard]] constexpr T ceil_div(T a, T b) noexcept {
    return (a + b - 1) / b;
}

template<Arithmetic T>
[[nodiscard]] constexpr T align_up(T value, T alignment) noexcept {
    return ((value + alignment - 1) / alignment) * alignment;
}

template<Arithmetic T>
[[nodiscard]] constexpr T align_down(T value, T alignment) noexcept {
    return (value / alignment) * alignment;
}

[[nodiscard]] inline bool is_power_of_two(u64 x) noexcept { return x && !(x & (x - 1)); }
[[nodiscard]] inline u64 next_power_of_two(u64 x) noexcept {
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}
[[nodiscard]] inline u64 prev_power_of_two(u64 x) noexcept {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x - (x >> 1);
}

template<Arithmetic T>
[[nodiscard]] constexpr T square(T x) noexcept { return x * x; }

template<Arithmetic T>
[[nodiscard]] constexpr T cube(T x) noexcept { return x * x * x; }

template<Arithmetic T>
[[nodiscard]] constexpr T mean(std::span<const T> values) noexcept {
    if (values.empty()) return T(0);
    T sum = std::accumulate(values.begin(), values.end(), T(0));
    return sum / static_cast<T>(values.size());
}

template<Arithmetic T>
[[nodiscard]] f64 variance(std::span<const T> values) noexcept {
    if (values.size() < 2) return 0.0;
    f64 m = static_cast<f64>(mean(values));
    f64 sum = 0.0;
    for (T v : values) {
        f64 d = static_cast<f64>(v) - m;
        sum += d * d;
    }
    return sum / static_cast<f64>(values.size() - 1);
}

template<Arithmetic T>
[[nodiscard]] f64 stddev(std::span<const T> values) noexcept {
    return std::sqrt(variance(values));
}

template<Arithmetic T>
[[nodiscard]] constexpr T median(std::span<T> values) noexcept {
    if (values.empty()) return T(0);
    std::sort(values.begin(), values.end());
    u32 n = values.size();
    return n % 2 == 0 ? (values[n/2 - 1] + values[n/2]) / 2 : values[n/2];
}

template<Arithmetic T>
[[nodiscard]] constexpr T percentile(std::span<T> values, f32 p) noexcept {
    if (values.empty()) return T(0);
    std::sort(values.begin(), values.end());
    u32 n = values.size();
    f32 idx = p * (n - 1);
    u32 i = static_cast<u32>(idx);
    f32 frac = idx - i;
    if (i + 1 >= n) return values[n - 1];
    return static_cast<T>(lerp(static_cast<f32>(values[i]), static_cast<f32>(values[i + 1]), frac));
}

} // namespace math

using math::clamp;
using math::lerp;
using math::smoothstep;
using math::smootherstep;
using math::deg2rad;
using math::rad2deg;
using math::abs;
using math::sign;
using math::floor_div;
using math::ceil_div;
using math::align_up;
using math::align_down;
using math::is_power_of_two;
using math::next_power_of_two;
using math::prev_power_of_two;
using math::square;
using math::cube;
using math::mean;
using math::variance;
using math::stddev;
using math::median;
using math::percentile;

} // namespace game_req