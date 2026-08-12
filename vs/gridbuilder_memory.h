#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class GridBuilderMemory
{
public:
    bool captureSnapshot();

    std::vector<size_t>
        changedAddresses() const;

    bool hasSnapshot() const;

    const std::vector<uint8_t>&
        snapshot() const;

    void refineChangedAddresses(
        uint8_t expectedDifference
    );

    const std::vector<size_t>&
        candidateAddresses() const;

private:
    std::vector<uint8_t>
        m_snapshot;

    std::vector<uint8_t>
        m_previousSnapshot;

    std::vector<size_t>
        m_candidateAddresses;
};
