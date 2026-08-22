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

    LinearPt previousInstructionAddress = 0;

    uint16_t previousCS = 0;
    uint16_t previousIP = 0;

    bool hasPreviousInstruction = false;

    LinearPt transitionTargetAddress = 0xEC2B;

    struct InstructionTransition
    {
        LinearPt previousAddress = 0;
        LinearPt currentAddress = 0;

        uint16_t previousCS = 0;
        uint16_t previousIP = 0;
    };

    std::vector<InstructionTransition>
        recordedInstructionTransitions;

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

        recordedInstructionTransitions.clear();

        previousInstructionAddress = 0;
        hasPreviousInstruction = false;
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

    recordedInstructionTransitions.clear();

    previousInstructionAddress = 0;
    hasPreviousInstruction = false;
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

void MemoryReadTracker::recordInstruction(
    LinearPt instructionAddress,
    uint16_t cs,
    uint16_t ip
)
{
    if(!trackingActive.load())
    {
        return;
    }

    if(instructionAddress == transitionTargetAddress &&
        hasPreviousInstruction)
    {
        std::lock_guard<std::mutex> lock(
            readAddressesMutex
        );

        recordedInstructionTransitions.push_back(
            {
                previousInstructionAddress,
                instructionAddress,
                previousCS,
                previousIP
            }
        );
    }

    previousInstructionAddress =
        instructionAddress;

    previousCS = cs;
    previousIP = ip;

    hasPreviousInstruction = true;
}

std::vector<std::pair<LinearPt, LinearPt>>
MemoryReadTracker::instructionTransitions()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<std::pair<LinearPt, LinearPt>>
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        result.emplace_back(
            transition.previousAddress,
            transition.currentAddress
        );
    }

    return result;
}

std::vector<std::pair<uint16_t, uint16_t>>
MemoryReadTracker::instructionTransitionContexts()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<std::pair<uint16_t, uint16_t>>
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        result.emplace_back(
            transition.previousCS,
            transition.previousIP
        );
    }

    return result;
}

void MemoryReadTracker::setTransitionTarget(
    LinearPt address
)
{
    transitionTargetAddress =
        address;
}

LinearPt MemoryReadTracker::transitionTarget()
{
    return transitionTargetAddress;
}
