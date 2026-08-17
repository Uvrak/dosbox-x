#pragma once

#include "mem.h"
#include <vector>

namespace MemoryReadTracker
{
    void record(LinearPt address);

    void start();
    void stop();

    bool active();

    std::vector<LinearPt> addresses();
}
