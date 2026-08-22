#pragma once

#include "mem.h"

#include <vector>
#include <utility>

namespace MemoryReadTracker
{
    void record(
        LinearPt address
    );

    void record(
        LinearPt address,
        LinearPt instructionAddress
    );

    void setTransitionTarget(
        LinearPt address
    );

    LinearPt transitionTarget();

    void start();
    void stop();
    void clear();

    bool active();

    std::vector<LinearPt>
        addresses();

    std::vector<std::pair<LinearPt, LinearPt>>
        instructions();

    void recordInstruction(
        LinearPt instructionAddress,
        uint16_t cs,
        uint16_t ip
    );

    std::vector<std::pair<LinearPt, LinearPt>>
        instructionTransitions();

    std::vector<std::pair<uint16_t, uint16_t>>
        instructionTransitionContexts();
}
