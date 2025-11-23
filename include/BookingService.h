#pragma once
#include "DataStore.h"

#include <memory>

namespace booking_service {

/**
 * @brief Thin wrapper around DataStore that exposes a clean service interface.
 */
class BookingService {
public:
    explicit BookingService(std::shared_ptr<DataStore> store);

    std::vector<Movie> GetMovies() const;
    std::vector<Theater> GetTheaters(int movieId) const;
    std::vector<Seat> GetSeats(int theaterId, int movieId) const;
    bool BookSeats(int theaterId, int movieId, const std::vector<std::string>& seatIds);

private:
    std::shared_ptr<DataStore> dataStore;
};

}  // namespace booking_service
