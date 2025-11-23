#include "BookingService.h"

namespace booking_service {

BookingService::BookingService(std::shared_ptr<DataStore> store)
    : dataStore(std::move(store)) {
}

std::vector<Movie> BookingService::GetMovies() const {
    return dataStore->GetMovies();
}

std::vector<Theater> BookingService::GetTheaters(int movieId) const {
    return dataStore->GetTheaters(movieId);
}

std::vector<Seat> BookingService::GetSeats(int theaterId, int movieId) const {
    return dataStore->GetSeats(theaterId, movieId);
}

bool BookingService::BookSeats(int theaterId, int movieId, const std::vector<std::string>& seatIds) {
    return dataStore->BookSeats(theaterId, movieId, seatIds);
}

}  // namespace booking_service