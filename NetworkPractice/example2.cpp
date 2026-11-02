#include <cstdint>
#include <stdexcept>
#include <vector>

using Bytes = std::vector<uint8_t>;

// uint32_t를 리틀 엔디안으로 기록한다.
void WriteU32LE(Bytes& bytes, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        // 하위 바이트부터 하나씩 배열에 추가한다.
        bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
    }
}

// signed 값도 같은 4바이트 패턴으로 기록한다.
// 음수는 uint32_t로 변환될 때 2^32를 기준으로 표현된다.
void WriteI32LE(Bytes& bytes, int32_t value)
{
    WriteU32LE(bytes, static_cast<uint32_t>(value));
}

// 리틀 엔디안으로 저장된 4바이트를 uint32_t로 조립한다.
uint32_t ReadU32LE(const uint8_t* bytes)
{
    uint32_t value = 0;

    for (int i = 0; i < 4; ++i)
    {
        value |= uint32_t(bytes[i]) << (i * 8);
    }

    return value;
}

// 32비트 비트 패턴을 int32_t 값으로 해석한다.
int32_t ReadI32LE(const uint8_t* bytes)
{
    const uint32_t raw = ReadU32LE(bytes);

    // 최상위 비트가 0이면 양수 범위다.
    if (raw <= 0x7FFFFFFFu)
        return static_cast<int32_t>(raw);

    // 최상위 비트가 1이면 음수로 해석한다.
    const int64_t signedValue =
        static_cast<int64_t>(raw) - (int64_t{ 1 } << 32);

    return static_cast<int32_t>(signedValue);
}

struct MoveRequest
{
    uint32_t mapId;
    int32_t x;
    int32_t y;
};

Bytes Serialize(const MoveRequest& request)
{
    Bytes bytes;
    bytes.reserve(12);

    // C# 코드와 같은 필드 순서로 기록한다.
    WriteU32LE(bytes, request.mapId);
    WriteI32LE(bytes, request.x);
    WriteI32LE(bytes, request.y);

    return bytes;
}

MoveRequest Deserialize(const Bytes& bytes)
{
    if (bytes.size() != 12)
        throw std::runtime_error("Invalid MoveRequest size");

    return MoveRequest{
        ReadU32LE(bytes.data()),
        ReadI32LE(bytes.data() + 4),
        ReadI32LE(bytes.data() + 8)
    };
}