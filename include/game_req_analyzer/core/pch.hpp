#pragma once

// Precompiled header for GameReqAnalyzer

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <bitset>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <ranges>
#include <regex>
#include <shared_mutex>
#include <source_location>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>

namespace game_req {

using json = nlohmann::json;
namespace fs = std::filesystem;
namespace chrono = std::chrono;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

using StringView = std::string_view;
using String = std::string;
using Path = fs::path;
using Json = json;

template<typename T>
using Opt = std::optional<T>;

using TimePoint = chrono::system_clock::time_point;
using Duration = chrono::nanoseconds;

inline constexpr u64 KiB = 1024;
inline constexpr u64 MiB = 1024 * 1024;
inline constexpr u64 GiB = 1024 * 1024 * 1024;
inline constexpr u64 TiB = 1024ull * 1024 * 1024 * 1024;

template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept StringLike = std::convertible_to<T, StringView>;

} // namespace game_req

// Include error handling after type definitions
#include <game_req_analyzer/core/error.hpp>
