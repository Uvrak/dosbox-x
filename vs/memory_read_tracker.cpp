#include "memory_read_tracker.h"

#include <unordered_set>
#include <vector>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>
#include <functional>

namespace
{
    std::atomic<bool> trackingActive{ false };

    std::unordered_set<LinearPt>
        readAddresses;

    struct ReadInstructionHash
    {
        size_t operator()(
            const std::pair<LinearPt, LinearPt>& value
            ) const noexcept
        {
            const size_t h1 =
                std::hash<LinearPt>{}(
                    value.first
                    );

            const size_t h2 =
                std::hash<LinearPt>{}(
                    value.second
                    );

            return h1 ^
                (h2 << 1);
        }
    };

    std::unordered_set<
        std::pair<LinearPt, LinearPt>,
        ReadInstructionHash
    >
        readInstructions;

    std::mutex
        readAddressesMutex;
}

void MemoryReadTracker::record(
    LinearPt address
)
{
    if(!trackingActive.load())
    {
        return;
    }

    constexpr LinearPt rangeStart = 0x2BF00;
    constexpr LinearPt rangeEnd = 0x2C100;

    if(address < rangeStart ||
        address > rangeEnd)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.insert(
        address
    );
}

void MemoryReadTracker::record(
    LinearPt address,
    LinearPt instructionAddress
)
{
    if(!trackingActive.load())
    {
        return;
    }

    constexpr LinearPt rangeStart = 0x2BF00;
    constexpr LinearPt rangeEnd = 0x2C100;

    if(address < rangeStart ||
        address > rangeEnd)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.insert(
        address
    );

    readInstructions.emplace(
        address,
        instructionAddress
    );
}

void MemoryReadTracker::start()
{
    {
        std::lock_guard<std::mutex> lock(
            readAddressesMutex
        );

        readAddresses.clear();
        readInstructions.clear();
    }

    trackingActive = true;
}

void MemoryReadTracker::stop()
{
    trackingActive = false;
}

void MemoryReadTracker::clear()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.clear();
    readInstructions.clear();
}

bool MemoryReadTracker::active()
{
    return trackingActive;
}

std::vector<LinearPt>
MemoryReadTracker::addresses()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<LinearPt> result(
        readAddresses.begin(),
        readAddresses.end()
    );

    std::sort(
        result.begin(),
        result.end()
    );

    return result;
}

std::vector<std::pair<LinearPt, LinearPt>>
MemoryReadTracker::instructions()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return std::vector<
        std::pair<LinearPt, LinearPt>
    >(
        readInstructions.begin(),
        readInstructions.end()
    );
}
