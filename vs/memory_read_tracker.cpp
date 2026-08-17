#include "memory_read_tracker.h"

#include <unordered_set>
#include <vector>
#include <algorithm>

namespace
{
    bool trackingActive = false;

    std::unordered_set<LinearPt>
        readAddresses;
}
void MemoryReadTracker::record(
    LinearPt address
)
{
    if(!trackingActive)
    {
        return;
    }

    readAddresses.insert(
        address
    );
}

void MemoryReadTracker::start()
{
    readAddresses.clear();
    trackingActive = true;
}

void MemoryReadTracker::stop()
{
    trackingActive = false;
}

bool MemoryReadTracker::active()
{
    return trackingActive;
}

std::vector<LinearPt>
MemoryReadTracker::addresses()
{
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
