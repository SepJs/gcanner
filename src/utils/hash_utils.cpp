#include <game_req_analyzer/utils/hash_utils.hpp>
#include <cstring>

namespace game_req {

u32 HashUtils::crc32(std::span<const u8> data, u32 crc) {
    static const u32 table[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
        0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
        0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
        0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
        0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
        0xFA0F3D63, 0x85080DFD, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
        0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x85080DFD, 0x136C9856, 0x646BA8C0,
        0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x85080DFD
    };
    
    crc = ~crc;
    for (u8 byte : data) {
        crc = table[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

u32 HashUtils::crc32c(std::span<const u8> data, u32 crc) {
    return crc32(data, crc);
}

std::array<u8, 16> HashUtils::md5(std::span<const u8> data) {
    std::array<u8, 16> result{};
    // Simplified - would use OpenSSL in production
    return result;
}

std::array<u8, 20> HashUtils::sha1(std::span<const u8> data) {
    std::array<u8, 20> result{};
    return result;
}

std::array<u8, 32> HashUtils::sha256(std::span<const u8> data) {
    std::array<u8, 32> result{};
    return result;
}

std::array<u8, 64> HashUtils::sha512(std::span<const u8> data) {
    std::array<u8, 64> result{};
    return result;
}

std::array<u8, 32> HashUtils::blake3(std::span<const u8> data) {
    std::array<u8, 32> result{};
    return result;
}

std::array<u8, 64> HashUtils::blake3_512(std::span<const u8> data) {
    std::array<u8, 64> result{};
    return result;
}

u64 HashUtils::xxhash64(std::span<const u8> data, u64 seed) {
    return 0;
}

u32 HashUtils::xxhash32(std::span<const u8> data, u32 seed) {
    return 0;
}

std::array<u8, 16> HashUtils::xxhash128(std::span<const u8> data, u64 seed) {
    std::array<u8, 16> result{};
    return result;
}

u64 HashUtils::fnv1a64(std::span<const u8> data) {
    u64 hash = 0xCBF29CE484222325;
    for (u8 byte : data) {
        hash ^= byte;
        hash *= 0x100000001B3;
    }
    return hash;
}

u32 HashUtils::fnv1a32(std::span<const u8> data) {
    u32 hash = 0x811C9DC5;
    for (u8 byte : data) {
        hash ^= byte;
        hash *= 0x01000193;
    }
    return hash;
}

u64 HashUtils::cityhash64(std::span<const u8> data) {
    return 0;
}

u128 HashUtils::cityhash128(std::span<const u8> data) {
    return 0;
}

u64 HashUtils::murmurhash64(std::span<const u8> data, u64 seed) {
    return 0;
}

u32 HashUtils::murmurhash32(std::span<const u8> data, u32 seed) {
    return 0;
}

u64 HashUtils::farmhash64(std::span<const u8> data) {
    return 0;
}

u128 HashUtils::farmhash128(std::span<const u8> data) {
    return 0;
}

String HashUtils::to_hex(std::span<const u8> hash) {
    static const char* hex = "0123456789abcdef";
    String result;
    result.reserve(hash.size() * 2);
    for (u8 byte : hash) {
        result.push_back(hex[byte >> 4]);
        result.push_back(hex[byte & 0xF]);
    }
    return result;
}

Result<std::vector<u8>> HashUtils::from_hex(StringView hex) {
    if (hex.size() % 2 != 0) return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid hex length"));
    std::vector<u8> result;
    result.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        char high = hex[i];
        char low = hex[i + 1];
        u8 byte = 0;
        if (high >= '0' && high <= '9') byte |= (high - '0') << 4;
        else if (high >= 'a' && high <= 'f') byte |= (high - 'a' + 10) << 4;
        else if (high >= 'A' && high <= 'F') byte |= (high - 'A' + 10) << 4;
        else return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid hex character"));
        
        if (low >= '0' && low <= '9') byte |= low - '0';
        else if (low >= 'a' && low <= 'f') byte |= low - 'a' + 10;
        else if (low >= 'A' && low <= 'F') byte |= low - 'A' + 10;
        else return make_unexpected(MAKE_ERROR(ErrorCode::InvalidArgument, "Invalid hex character"));
        
        result.push_back(byte);
    }
    return result;
}

bool HashUtils::constant_time_compare(std::span<const u8> a, std::span<const u8> b) {
    if (a.size() != b.size()) return false;
    u8 diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

class MD5Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 16; }
    String name() const override { return "MD5"; }
};

class SHA1Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 20; }
    String name() const override { return "SHA1"; }
};

class SHA256Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 32; }
    String name() const override { return "SHA256"; }
};

class SHA512Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 64; }
    String name() const override { return "SHA512"; }
};

class BLAKE3Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 32; }
    String name() const override { return "BLAKE3"; }
};

class CRC32Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 4; }
    String name() const override { return "CRC32"; }
};

class XXHash64Context : public HashContext {
public:
    void update(std::span<const u8>) override {}
    std::vector<u8> finalize() override { return {}; }
    void reset() override {}
    u32 digest_size() const override { return 8; }
    String name() const override { return "XXH64"; }
};

std::unique_ptr<HashContext> HashUtils::create_md5() { return std::make_unique<MD5Context>(); }
std::unique_ptr<HashContext> HashUtils::create_sha1() { return std::make_unique<SHA1Context>(); }
std::unique_ptr<HashContext> HashUtils::create_sha256() { return std::make_unique<SHA256Context>(); }
std::unique_ptr<HashContext> HashUtils::create_sha512() { return std::make_unique<SHA512Context>(); }
std::unique_ptr<HashContext> HashUtils::create_blake3() { return std::make_unique<BLAKE3Context>(); }
std::unique_ptr<HashContext> HashUtils::create_crc32() { return std::make_unique<CRC32Context>(); }
std::unique_ptr<HashContext> HashUtils::create_xxhash64() { return std::make_unique<XXHash64Context>(); }

} // namespace game_req
