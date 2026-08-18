#pragma once

#include "mem.h"
#include <vector>

namespace MemoryReadTracker
{
    void record(LinearPt address);

    void start();
    void stop();
    void clear();

    bool active();

    std::vector<LinearPt> addresses();
}
