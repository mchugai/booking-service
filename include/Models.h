#pragma once

#include <string>
#include <vector>

namespace booking_service {

/**
 * @brief Represents a single seat in a theater for a specific show.
 *
 * Seat identifiers are labels such as "a1", "a2", ...
 * The booking state is tracked by the backend and updated atomically when a
 * reservation request succeeds.
 */
struct Seat {
    std::string id;
    bool isBooked = false;
};

/**
 * @brief Represents a theater that can host movie shows.
 *
 * Each theater has a unique numeric id, a name, and a predefined list of seats.
 */
struct Theater {
    int id;
    std::string name;
    std::vector<Seat> seats;
};

/**
 * @brief Represents a movie that can be shown in theaters.
 *
 * Each movie has a unique numeric id and a title.
 */
struct Movie {
    int id;
    std::string title;
};

}  // namespace booking_service
