#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <queue>
#include <vector>

struct MovementAction
{
    uint64_t sequence = 0;
    uint64_t clientTick = 0;
    int32_t xMillimeters = 0;
};

struct MovementBatch
{
    uint32_t mapId = 0;
    uint64_t generation = 0;
    uint64_t batchSequence = 0;
    uint64_t latestClientTick = 0;
    std::vector<MovementAction> actions;
};

class MovementBatchScheduler
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    static constexpr auto SendInterval = std::chrono::milliseconds(200);
    static constexpr auto CheckpointInterval = std::chrono::seconds(1);

    static constexpr size_t MaxActionsPerBatch = 32;
    static constexpr size_t MaxPendingActions = 128;

    // 이동 액션을 전송 대기열에 추가한다.
    // false이면 대기열이 가득 찬 것이므로 호출한 쪽에서 처리해야 한다.
    bool TryEnqueue(const MovementAction& action)
    {
        if (_pending.size() >= MaxPendingActions)
            return false;

        _pending.push(action);
        return true;
    }

    // 전송 시각이 되면 대기 액션을 묶어 반환한다.
    // 액션이 없으면 체크포인트 시각에 현재 상태를 대신 넣는다.
    std::optional<MovementBatch> TryCreateBatch(
        TimePoint now,
        uint32_t mapId,
        uint64_t generation,
        uint64_t latestClientTick,
        const MovementAction& currentCheckpoint)
    {
        // 첫 호출 시 기준 시각을 설정한다.
        if (!_started)
        {
            _started = true;
            _nextSendAt = now;
            _nextCheckpointAt = now + CheckpointInterval;
        }

        // 200ms가 지나기 전에는 새 배치를 만들지 않는다.
        if (now < _nextSendAt)
            return std::nullopt;

        // 대기 액션이 없다면 1초 주기의 체크포인트까지 기다린다.
        if (_pending.empty())
        {
            if (now < _nextCheckpointAt)
                return std::nullopt;

            _pending.push(currentCheckpoint);
        }

        MovementBatch batch;
        batch.mapId = mapId;
        batch.generation = generation;
        batch.batchSequence = ++_batchSequence;
        batch.latestClientTick = latestClientTick;
        batch.actions.reserve(MaxActionsPerBatch);

        // 대기 중인 액션을 한 패킷에 최대 32개까지 담는다.
        while (!_pending.empty() &&
            batch.actions.size() < MaxActionsPerBatch)
        {
            batch.actions.push_back(_pending.front());
            _pending.pop();
        }

        // 밀린 주기를 따라잡으려고 패킷을 연속해서 보내지 않고,
        // 지금 시각 기준으로 다음 전송 시점을 예약한다.
        _nextSendAt = now + SendInterval;
        _nextCheckpointAt = now + CheckpointInterval;

        return batch;
    }

private:
    std::queue<MovementAction> _pending;

    bool _started = false;
    TimePoint _nextSendAt{};
    TimePoint _nextCheckpointAt{};

    uint64_t _batchSequence = 0;
};

int main()
{
    using Clock = MovementBatchScheduler::Clock;

    MovementBatchScheduler scheduler;
    const auto start = Clock::now();

    // 2초 동안 20ms 간격으로 이동 샘플을 만든다고 가정한다.
    // 실제 게임에서는 입력 변경, 점프, 착지 등의 시점에 액션을 추가할 수 있다.
    for (uint64_t tick = 0; tick < 100; ++tick)
    {
        const auto now = start + std::chrono::milliseconds(tick * 20);

        MovementAction action;
        action.sequence = tick + 1;
        action.clientTick = tick + 1;
        action.xMillimeters = static_cast<int32_t>(tick * 40);

        if (!scheduler.TryEnqueue(action))
        {
            std::cerr << "Movement queue is full\n";
            return 1;
        }

        MovementAction checkpoint = action;

        auto batch = scheduler.TryCreateBatch(now,100000002,1,tick + 1,checkpoint);

        if (batch)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

            std::cout << "time=" << elapsed<< "ms, batch=" << batch->batchSequence << ", actions=" << batch->actions.size() << '\n';

            // 실제 프로그램에서는 여기서:
            // 1. batch를 패킷 Payload로 직렬화하고
            // 2. Network Session으로 전송한다.
        }
    }

    return 0;
}