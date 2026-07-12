#pragma once

#include <game_req_analyzer/core/pch.hpp>
#include <game_req_analyzer/core/expected.hpp>

namespace game_req {

enum class ErrorCode : u32 {
    Success = 0,
    FileNotFound,
    PermissionDenied,
    InvalidPath,
    InvalidFileFormat,
    CorruptedFile,
    UnsupportedFormat,
    OutOfMemory,
    IoError,
    ParseError,
    NetworkError,
    Timeout,
    Cancelled,
    NotImplemented,
    InvalidArgument,
    DatabaseError,
    UpdateFailed,
    ChecksumMismatch,
    CompressionError,
    EncryptionError,
    Unknown
};

struct Error {
    ErrorCode code = ErrorCode::Unknown;
    String message;
    StringView function_name;
    u32 line = 0;
    
    Error() = default;
    Error(ErrorCode c, StringView msg, StringView func, u32 ln)
        : code(c), message(msg), function_name(func), line(ln) {}
    
    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Success; }
    [[nodiscard]] String to_string() const noexcept {
        return std::format("[{}] {} ({}:{})", 
            static_cast<u32>(code), message, function_name, line);
    }
};

#define MAKE_ERROR(code, msg) \
    Error(code, msg, __func__, __LINE__)

template<typename T>
using Result = Expected<T, Error>;

inline Result<void> success() { return {}; }

template<typename T>
inline Result<T> success(const T& value) { return Result<T>(value); }

template<typename T>
inline Result<T> success(T&& value) { return Result<T>(std::forward<T>(value)); }

} // namespace game_req