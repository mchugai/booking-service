#include "DataStore.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace booking_service {

namespace {

using json = nlohmann::json;

constexpr std::string_view kMoviesFile = "movies.json";
constexpr std::string_view kTheatersFile = "theaters.json";
constexpr std::string_view kMappingsFile = "mappings.json";

std::optional<json> LoadJson(const fs::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << path << std::endl;
        return std::nullopt;
    }
    json j;
    try {
        file >> j;
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error in " << path << ": " << e.what() << std::endl;
        return std::nullopt;
    }
    return j;
}

}  // namespace

void DataStore::LoadData(const fs::path& dataDir) {
    const auto moviesJson = LoadJson(dataDir / kMoviesFile);
    if (!moviesJson) {
        throw std::runtime_error("Failed to load " + std::string(kMoviesFile));
    }

    mapMovies.clear();
    for (const auto& item : *moviesJson) {
        if (!item.contains("id") || !item.contains("title")) {
            std::cerr << "[DataStore] Skipping movie with missing fields\n";
            continue;
        }

        int id = item["id"].get<int>();
        std::string title = item["title"].get<std::string>();
        mapMovies.emplace(id, Movie{id, std::move(title)});
    }

    const auto theatersJson = LoadJson(dataDir / kTheatersFile);
    if (!theatersJson) {
        throw std::runtime_error("Failed to load " + std::string(kTheatersFile));
    }

    mapTheaters.clear();
    for (const auto& item : *theatersJson) {
        if (!item.contains("id") || !item.contains("name") || !item.contains("capacity")) {
            std::cerr << "[DataStore] Skipping theater with missing fields\n";
            continue;
        }

        Theater t;
        t.id = item["id"].get<int>();
        t.name = item["name"].get<std::string>();

        const int capacity = item["capacity"].get<int>();
        if (capacity <= 0) {
            std::cerr << "[DataStore] Theater " << t.id << " has non-positive capacity, skipping\n";
            continue;
        }

        t.seats.reserve(static_cast<std::size_t>(capacity));
        for (int i = 1; i <= capacity; ++i) {
            t.seats.push_back(Seat{"a" + std::to_string(i), false});
        }
        mapTheaters.emplace(t.id, std::move(t));
    }

    const auto mappingsJson = LoadJson(dataDir / kMappingsFile);
    if (!mappingsJson) {
        throw std::runtime_error("Failed to load " + std::string(kMappingsFile));
    }

    mapMovieTheaters.clear();
    for (auto& [movieIdStr, theaterIds] : mappingsJson->items()) {
        int movieId = 0;
        try {
            movieId = std::stoi(movieIdStr);
        }
        catch (const std::exception& e) {
            std::cerr << "[DataStore] Invalid movieId key in mappings: " << movieIdStr << " (" << e.what() << ")\n";
            continue;
        }

        if (!mapMovies.contains(movieId)) {
            std::cerr << "[DataStore] Mapping for unknown movieId " << movieId << " – skipping\n";
            continue;
        }

        std::vector<int> tids;
        if (theaterIds.is_array()) {
            tids.reserve(theaterIds.size());
            for (const auto& tid : theaterIds) {
                int theaterId = tid.get<int>();
                if (!mapTheaters.contains(theaterId)) {
                    std::cerr << "[DataStore] Mapping movie " << movieId << " to unknown theaterId " << theaterId
                              << " – skipping this theater\n";
                    continue;
                }
                tids.push_back(theaterId);
            }
        }

        if (!tids.empty()) {
            mapMovieTheaters.emplace(movieId, std::move(tids));
        }
    }

    mapShows.clear();
    for (const auto& [movieId, theaterIds] : mapMovieTheaters) {
        for (const int tid : theaterIds) {
            auto it = mapTheaters.find(tid);
            if (it == mapTheaters.end()) {
                std::cerr << "[DataStore] Internal inconsistency: theater " << tid << " not found when creating show\n";
                continue;
            }

            auto show = std::make_unique<Show>();
            show->seats = it->second.seats;
            mapShows.emplace(std::make_pair(movieId, tid), std::move(show));
        }
    }
}

std::vector<Movie> DataStore::GetMovies() const {
    std::vector<Movie> result;
    result.reserve(mapMovies.size());
    std::ranges::copy(mapMovies | std::views::values, std::back_inserter(result));
    return result;
}

std::vector<Theater> DataStore::GetTheaters(int movieId) const {
    std::vector<Theater> result;
    auto it = mapMovieTheaters.find(movieId);
    if (it == mapMovieTheaters.end()) {
        return result;
    }

    auto theatersView = it->second | std::views::transform([this](const int tid) { return mapTheaters.find(tid); }) |
                        std::views::filter([this](const auto& tit) { return tit != mapTheaters.end(); }) |
                        std::views::transform([](const auto& tit) { return tit->second; });

    std::ranges::copy(theatersView, std::back_inserter(result));
    return result;
}

std::optional<Theater> DataStore::GetTheater(int theaterId) const {
    if (auto it = mapTheaters.find(theaterId); it != mapTheaters.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool DataStore::BookSeats(int theaterId, int movieId, const std::vector<std::string>& seatIds) {
    if (seatIds.empty()) {
        return false;
    }

    auto it = mapShows.find({movieId, theaterId});
    if (it == mapShows.end()) {
        return false;
    }

    auto& show = *it->second;
    std::lock_guard lock(show.mtx);

    std::vector<Seat*> seatsToBook;
    seatsToBook.reserve(seatIds.size());

    for (const auto& seatId : seatIds) {
        auto seatIt = std::ranges::find_if(show.seats, [&seatId](const auto& seat) { return seat.id == seatId; });

        if (seatIt == show.seats.end()) {
            return false;
        }
        if (seatIt->isBooked) {
            return false;
        }
        seatsToBook.push_back(&(*seatIt));
    }

    for (auto* seat : seatsToBook) {
        seat->isBooked = true;
    }

    return true;
}

std::vector<Seat> DataStore::GetSeats(int theaterId, int movieId) const {
    auto it = mapShows.find({movieId, theaterId});
    if (it == mapShows.end()) {
        return {};
    }

    auto& show = *it->second;
    std::lock_guard lock(show.mtx);
    return show.seats;
}

}  // namespace booking_service
