#pragma once

#include <cstdint>

enum class MightAndMagic1Direction
{
    North,
    East,
    South,
    West,
    Unknown
};

struct MightAndMagic1State
{
    int x = 0;
    int y = 0;

    int areaId = 0;

    MightAndMagic1Direction direction =
        MightAndMagic1Direction::Unknown;

    bool valid = false;
};

class MightAndMagic1
{
public:
    static MightAndMagic1State readState();

private:
    static constexpr size_t
        PositionAxisAAddress = 116248;

    static constexpr size_t
        PositionAxisBAddress = 116249;

    static constexpr size_t
        DirectionAddress = 116253;

    // Unconfirmed candidate.
    // This address is not stable during live polling
    // and must be replaced after further scanning.
    static constexpr size_t
        AreaIdAddress = 116265;

    static MightAndMagic1Direction
        decodeDirection(
            uint8_t value
        );
};
