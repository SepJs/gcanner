#pragma once

#include <game_req_analyzer/core/pch.hpp>

namespace game_req {

class HashUtils {
public:
    static u32 crc32(std::span<const u8> data, u32 crc = 0xFFFFFFFF);
    static u32 crc32c(std::span<const u8> data, u32 crc = 0xFFFFFFFF);
    
    static std::array<u8, 16> md5(std::span<const u8> data);
    static std::array<u8, 20> sha1(std::span<const u8> data);
    static std::array<u8, 32> sha256(std::span<const u8> data);
    static std::array<u8, 64> sha512(std::span<const u8> data);
    
    static std::array<u8, 32> blake3(std::span<const u8> data);
    static std::array<u8, 64> blake3_512(std::span<const u8> data);
    
    static u64 xxhash64(std::span<const u8> data, u64 seed = 0);
    static u32 xxhash32(std::span<const u8> data, u32 seed = 0);
    static std::array<u8, 16> xxhash128(std::span<const u8> data, u64 seed = 0);
    
    static u64 fnv1a64(std::span<const u8> data);
    static u32 fnv1a32(std::span<const u8> data);
    
    static u64 cityhash64(std::span<const u8> data);
    static u128 cityhash128(std::span<const u8> data);
    
    static u64 murmurhash64(std::span<const u8> data, u64 seed = 0);
    static u32 murmurhash32(std::span<const u8> data, u32 seed = 0);
    
    static u64 farmhash64(std::span<const u8> data);
    static u128 farmhash128(std::span<const u8> data);
    
    static String to_hex(std::span<const u8> hash);
    static Result<std::vector<u8>> from_hex(StringView hex);
    
    static bool constant_time_compare(std::span<const u8> a, std::span<const u8> b);
    
    struct HashContext {
        virtual ~HashContext() = default;
        virtual void update(std::span<const u8> data) = 0;
        virtual std::vector<u8> finalize() = 0;
        virtual void reset() = 0;
        [[nodiscard]] virtual u32 digest_size() const = 0;
        [[nodiscard]] virtual String name() const = 0;
    };
    
    static std::unique_ptr<HashContext> create_md5();
    static std::unique_ptr<HashContext> create_sha1();
    static std::unique_ptr<HashContext> create_sha256();
    static std::unique_ptr<HashContext> create_sha512();
    static std::unique_ptr<HashContext> create_blake3();
    static std::unique_ptr<HashContext> create_crc32();
    static std::unique_ptr<HashContext> create_xxhash64();
};

} // namespace game_req