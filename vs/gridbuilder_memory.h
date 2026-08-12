#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Might and Magic 1 memory findings:
//
// Facing direction byte:
// Physical address: 116253 (0x1C61D)
//
// Values while turning right:
// 12 -> 3 -> 192 -> 48 -> 12
//
// The absolute mapping to North/East/South/West
// still needs to be determined.

// Might and Magic 1 position:
//
// Position axis A:
// Physical address: 116248 (0x1C618)
// Changes by 1 per map field.
//
// Position axis B:
// Physical address: 116249 (0x1C619)
// Changes by 1 per map field.
//
// Which axis corresponds to GridBuilder X/Y and whether either
// direction must be inverted still needs to be determined.
//
// Related value:
// 116250 (0x1C61A) changed together with axis A during testing,
// but is not required for the raw position.

class GridBuilderMemory
{
public:
    bool captureSnapshot();

    std::vector<size_t>
        changedAddresses() const;

    bool hasSnapshot() const;

    const std::vector<uint8_t>&
        snapshot() const;

    void resetCandidates();

    void refineChangedAddresses(
        const std::vector<uint8_t>&
        expectedDifferences
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

    bool m_candidatesInitialized =
        false;
};
