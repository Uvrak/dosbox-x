#include "memory_read_tracker.h"

#include <unordered_set>
#include <vector>
#include <algorithm>
#include <atomic>
#include <mutex>

namespace
{
    std::atomic<bool> trackingActive{ false };

    std::unordered_set<LinearPt>
        readAddresses;

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

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.insert(
        address
    );

    (void)instructionAddress;
}

void MemoryReadTracker::start()
{
    {
        std::lock_guard<std::mutex> lock(
            readAddressesMutex
        );

        readAddresses.clear();
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
