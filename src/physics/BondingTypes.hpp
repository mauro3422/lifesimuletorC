#ifndef BONDING_TYPES_HPP
#define BONDING_TYPES_HPP

// Shared definitions for the Bonding Subsystem to avoid circular includes
enum class BondError {
    SUCCESS = 0,
    VALENCY_FULL,
    DISTANCE_TOO_FAR,
    ANGLE_INCOMPATIBLE,
    ALREADY_CLUSTERED,
    ALREADY_BONDED,
    INTERNAL_ERROR
};

#endif // BONDING_TYPES_HPP
