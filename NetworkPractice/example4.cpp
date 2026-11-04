#include <algorithm>
#include <cmath>

struct Vec2
{
    float x;
    float y;
};

struct PlayerState
{
    Vec2 position;        // 서버가 현재 인정하는 위치
    float maxSpeed;       // 초당 최대 이동 거리
};

enum class MoveResult
{
    Accepted,
    InvalidNumber,        // NaN 또는 무한대
    OutOfWorld,           // 월드 범위 초과
    TooFast               // 허용 속도보다 빠른 이동
};

MoveResult ValidateAndApplyMove(
    PlayerState& player,
    const Vec2& requestedPosition,
    float elapsedSeconds)
{
    // 좌표에 NaN이나 무한대가 들어오면 거부한다.
    if (!std::isfinite(requestedPosition.x) ||
        !std::isfinite(requestedPosition.y))
    {
        return MoveResult::InvalidNumber;
    }

    // 예시 월드 범위 검사
    constexpr float worldMin = -1000.0f;
    constexpr float worldMax = 1000.0f;

    if (requestedPosition.x < worldMin ||
        requestedPosition.x > worldMax ||
        requestedPosition.y < worldMin ||
        requestedPosition.y > worldMax)
    {
        return MoveResult::OutOfWorld;
    }

    // 잘못된 시간값으로 이동 허용 거리가 무한히 커지지 않게 제한한다.
    constexpr float maxElapsedSeconds = 0.5f;
    elapsedSeconds = std::clamp(elapsedSeconds, 0.0f, maxElapsedSeconds);

    const float dx = requestedPosition.x - player.position.x;
    const float dy = requestedPosition.y - player.position.y;
    const float distance = std::sqrt(dx * dx + dy * dy);

    // 네트워크 지연이나 좌표 오차를 고려한 작은 여유 거리
    constexpr float tolerance = 0.25f;
    const float allowedDistance =
        player.maxSpeed * elapsedSeconds + tolerance;

    if (distance > allowedDistance)
    {
        return MoveResult::TooFast;
    }

    // 검증을 통과한 경우에만 서버 상태를 갱신한다.
    player.position = requestedPosition;
    return MoveResult::Accepted;
}