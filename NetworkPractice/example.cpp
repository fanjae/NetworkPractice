#include <bit>
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <iostream>


using Bytes = std::vector<uint8_t>;

// uint32_t 값을 리틀 엔디안 순서로 바이트 배열 뒤에 추가한다.
// 가장 낮은 8비트부터 한 바이트씩 기록한다.
void WriteU32LE(Bytes& bytes, uint32_t value)
{
    for (int i = 0; i < 4; ++i)
    {
        // i가 0이면 하위 8비트, i가 1이면 그 다음 8비트를 꺼낸다.
        // 예: 0x000003E8 -> E8, 03, 00, 00
        bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
    }
}

// 음수도 포함하는 int32_t를 기록한다.
// bit_cast는 숫자의 비트 패턴을 그대로 uint32_t로 해석하게 한다.
void WriteI32LE(Bytes& bytes, int32_t value)
{
    WriteU32LE(bytes, std::bit_cast<uint32_t>(value));
}

// 리틀 엔디안 바이트 4개를 uint32_t 값 하나로 조립한다.
uint32_t ReadU32LE(const uint8_t* bytes)
{
    uint32_t value = 0;

    for (int i = 0; i < 4; ++i)
    {
        // 각 바이트를 원래 위치에 맞춰 이동한 뒤 결과에 합친다.
        value |= uint32_t(bytes[i]) << (i * 8);
    }

    return value;
}

// uint32_t로 읽은 비트 패턴을 int32_t로 다시 해석한다.
int32_t ReadI32LE(const uint8_t* bytes)
{
    return std::bit_cast<int32_t>(ReadU32LE(bytes));
}

struct MoveRequest
{
    uint32_t mapId;
    int32_t x;
    int32_t y;
};

// 객체의 필드를 약속한 순서와 형식으로 바이트 배열에 기록한다.
Bytes Serialize(const MoveRequest& request)
{
    Bytes bytes;

    // mapId, x, y가 각각 4바이트이므로 전체 Payload는 12바이트다.
    bytes.reserve(12);

    WriteU32LE(bytes, request.mapId); // Payload의 0~3번 바이트
    WriteI32LE(bytes, request.x);     // Payload의 4~7번 바이트
    WriteI32LE(bytes, request.y);     // Payload의 8~11번 바이트

    return bytes;
}

// 바이트 배열을 같은 순서와 형식으로 읽어 MoveRequest를 복원한다.
MoveRequest Deserialize(const Bytes& bytes)
{
    // 정해진 Payload 크기와 다르면 불완전하거나 잘못된 데이터로 처리한다.
    if (bytes.size() != 12)
        throw std::runtime_error("Invalid MoveRequest size");

    return MoveRequest{
        ReadU32LE(bytes.data()),     // 0번 위치부터 4바이트를 mapId로 읽는다.
        ReadI32LE(bytes.data() + 4), // 4번 위치부터 4바이트를 x로 읽는다.
        ReadI32LE(bytes.data() + 8)  // 8번 위치부터 4바이트를 y로 읽는다.
    };
}

// 프로그램 실행이 시작되는 진입점
int main()
{
    MoveRequest original{ 100000002, 1250, -400 };

    // 객체를 바이트 배열로 직렬화
    Bytes bytes = Serialize(original);

    // 바이트 배열을 객체로 역직렬화
    MoveRequest restored = Deserialize(bytes);

    std::cout << "Payload size: " << bytes.size() << " bytes\n";
    std::cout << "Map ID: " << restored.mapId << '\n';
    std::cout << "Position: (" << restored.x << ", " << restored.y << ")\n";

    return 0;
}
