#include "BookingService.h"
#include "DataStore.h"

#include <iostream>
#include <algorithm>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

using namespace booking_service;

namespace {

constexpr std::string_view kPathData = "data";

void PrintMovies(const std::vector<Movie>& movies) {
    std::cout << "\nAvailable Movies:\n";
    int idx = 1;
    for (const auto& movie : movies) {
        std::cout << "[" << idx++ << "] " << movie.title << "\n";
    }
    std::cout << std::endl;
}

void PrintTheaters(const std::vector<Theater>& theaters) {
    std::cout << "\nTheaters showing the movie:\n";
    int idx = 1;
    for (const auto& theater : theaters) {
        std::cout << "[" << idx++ << "] " << theater.name << "\n";
    }
    std::cout << std::endl;
}

void PrintSeats(const std::vector<Seat>& seats) {
    std::cout << "\nAvailable Seats:\n";
    int count = 0;
    for (const auto& seat : seats) {
        if (!seat.isBooked) {
            std::cout << seat.id << " ";
            count++;
            if (count % 10 == 0) {
                std::cout << "\n";
            }
        }
    }
    std::cout << "\n\nTotal available: " << count << "\n" << std::endl;
}

}  // namespace

int main() {
    auto store = std::make_shared<DataStore>();
    store->LoadData(kPathData);
    BookingService service(store);

    std::cout << "Welcome to Movie Booking Service CLI\n";

    while (true) {
        std::cout << "1. List Movies\n";
        std::cout << "2. Exit\n";
        std::cout << "Enter choice: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 2) {
            break;
        }

        if (choice == 1) {
            auto movies = service.GetMovies();
            PrintMovies(movies);

            std::cout << "Enter Movie ID or Index to view theaters (or 'b' to back): ";
            std::string input;
            std::cin >> input;
            if (input == "b") {
                continue;
            }

            int movieId = 0;
            try {
                int idx = std::stoi(input);
                if (idx >= 1 && idx <= static_cast<int>(movies.size())) {
                    movieId = movies[idx - 1].id;
                }
            }
            catch (...) {
                std::cout << "Invalid input.\n";
                continue;
            }

            auto theaters = service.GetTheaters(movieId);
            if (theaters.empty()) {
                std::cout << "No theaters found for this movie or invalid ID.\n";
                continue;
            }
            PrintTheaters(theaters);

            std::cout << "Enter Theater ID or Index to view seats (or 'b' to back): ";
            std::string tInput;
            std::cin >> tInput;
            if (tInput == "b") {
                continue;
            }

            int theaterId = 0;
            try {
                int idx = std::stoi(tInput);
                if (idx >= 1 && idx <= static_cast<int>(theaters.size())) {
                    theaterId = theaters[idx - 1].id;
                }
            }
            catch (...) {
                std::cout << "Invalid input.\n";
                continue;
            }

            auto seats = service.GetSeats(theaterId, movieId);
            if (seats.empty()) {
                std::cout << "No seats found or invalid Theater ID.\n";
                continue;
            }
            PrintSeats(seats);

            std::cout << "Enter seat IDs to book (space separated, e.g., a1 a2) or "
                         "'b' to back: ";

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::string line;
            std::getline(std::cin, line);
            if (line == "b") {
                continue;
            }

            std::replace(line.begin(), line.end(), ',', ' ');

            std::stringstream ss(line);
            std::string seatId;
            std::vector<std::string> seatsToBook;
            while (ss >> seatId) {
                seatsToBook.push_back(seatId);
            }

            if (seatsToBook.empty()) {
                continue;
            }

            if (service.BookSeats(theaterId, movieId, seatsToBook)) {
                std::cout << "Booking SUCCESSFUL!\n";
            }
            else {
                std::cout << "Booking FAILED! Some seats might be already booked or "
                             "invalid.\n";
            }
        }
    }

    return 0;
}
