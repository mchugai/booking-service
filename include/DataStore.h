#pragma once
#include "Models.h"

#include <map>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace booking_service {

/**
 * @brief In-memory storage for movies, theaters and seat bookings.
 *  - Movies, theaters and mappings are loaded once and never change.
 *  - Each (movieId, theaterId) pair has its own Show with its own seat state.
 *  - Show objects use per-show mutexes, so different shows can be booked in parallel.
 */
class DataStore {
public:
    DataStore() = default;

    /**
     * @brief Loads movies, theaters and movie→theaters mappings from JSON files.
     *
     * Expected JSON files:
     *  - movies.json
     *  - theaters.json
     *  - mappings.json
     *
     * After loading, the static data (movies, theaters, mappings) does not change.
     * Seat states for each show are initialized based on theater capacity.
     * @param dataDir Path to directory containing JSON configuration files.
     */
    void LoadData(const fs::path& dataDir);

    /**
     * @brief Returns all available movies.
     *
     * @return Vector of Movie objects.
     */
    std::vector<Movie> GetMovies() const;

    /**
     * @brief Returns all theaters that show the given movie.
     *
     * @param movieId Numeric ID of the movie.
     * @return Vector of Theater objects. Empty if movie does not exist or no theaters are mapped.
     */
    std::vector<Theater> GetTheaters(int movieId) const;

    /**
     * @brief Retrieves a theater by ID.
     *
     * @param theaterId Numeric ID of the theater.
     * @return Optional containing Theater if found, otherwise std::nullopt.
     */
    std::optional<Theater> GetTheater(int theaterId) const;

    /**
     * @brief Books the given seats for a specific (movieId, theaterId) show.
     *  - All seats must exist.
     *  - None of them may already be booked.
     *  - The operation is atomic: if one seat fails, nothing is booked.
     *
     * @return true on success, false otherwise.
     */
    bool BookSeats(int theaterId, int movieId, const std::vector<std::string>& seatIds);

    /**
     * @brief Returns a thread-safe copy of seats for a specific show.
     *
     * @param theaterId Theater ID.
     * @param movieId Movie ID.
     * @return Vector of Seat objects. Empty vector if show does not exist.
     */
    std::vector<Seat> GetSeats(int theaterId, int movieId) const;

private:
    /**
     * @brief Internal representation of a single movie show in a particular theater.
     *   - A local seat state (copy of Theater.seats)
     *   - A mutex for protecting modifications
     * Each Show corresponds uniquely to a (<movieId>, <theaterId>) pair.
     */
    struct Show {
        std::vector<Seat> seats;
        mutable std::mutex mtx;
    };

    struct PairHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;
        }
    };

    std::map<int, Movie> mapMovies;
    std::map<int, std::vector<int>> mapMovieTheaters;
    std::map<int, Theater> mapTheaters;
    std::unordered_map<std::pair<int, int>, std::unique_ptr<Show>, PairHash> mapShows;
};

}  // namespace booking_service
