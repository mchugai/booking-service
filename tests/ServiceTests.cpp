#include "BookingService.h"
#include "DataStore.h"

#include <gtest/gtest.h>

#include <future>
#include <thread>
#include <vector>

using namespace booking_service;

class BookingServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        store = std::make_shared<DataStore>();
        // In a real scenario, we want to load test-specific data
        // For now: we assume running from project root and load the default data
        store->LoadData("data");
        service = std::make_unique<BookingService>(store);
    }

    std::shared_ptr<DataStore> store;
    std::unique_ptr<BookingService> service;
};

TEST_F(BookingServiceTest, GetMoviesReturnsMovies) {
    auto movies = service->GetMovies();
    ASSERT_FALSE(movies.empty());
    EXPECT_EQ(movies.size(), 4);
}

TEST_F(BookingServiceTest, GetTheatersReturnsTheatersForMovie) {
    auto theaters = service->GetTheaters(1);
    ASSERT_EQ(theaters.size(), 2);
}

TEST_F(BookingServiceTest, GetSeatsReturnsSeats) {
    auto seats = service->GetSeats(1, 1);
    ASSERT_EQ(seats.size(), 20);
    EXPECT_EQ(seats[0].id, "a1");
}

TEST_F(BookingServiceTest, BookSeatsSuccess) {
    std::vector<std::string> seatsToBook = {"a1", "a2"};
    bool success = service->BookSeats(1, 1, seatsToBook);
    EXPECT_TRUE(success);

    auto seats = service->GetSeats(1, 1);
    for (const auto& seat : seats) {
        if (seat.id == "a1" || seat.id == "a2") {
            EXPECT_TRUE(seat.isBooked);
        }
    }
}

TEST_F(BookingServiceTest, BookSeatsFailureAlreadyBooked) {
    std::vector<std::string> seatsToBook = {"a1"};
    EXPECT_TRUE(service->BookSeats(1, 1, seatsToBook));
    EXPECT_FALSE(service->BookSeats(1, 1, seatsToBook));
}

TEST_F(BookingServiceTest, BookSeatsFailureInvalidSeat) {
    std::vector<std::string> seatsToBook = {"z99"};
    EXPECT_FALSE(service->BookSeats(1, 1, seatsToBook));
}

TEST_F(BookingServiceTest, GetTheatersInvalidMovie) {
    auto theaters = service->GetTheaters(9999);
    EXPECT_TRUE(theaters.empty());
}

TEST_F(BookingServiceTest, GetSeatsInvalidTheater) {
    auto seats = service->GetSeats(9999, 1);
    EXPECT_TRUE(seats.empty());
}

TEST_F(BookingServiceTest, GetSeatsInvalidMovie) {
    auto seats = service->GetSeats(1, 9999);
    EXPECT_TRUE(seats.empty());
}

TEST_F(BookingServiceTest, BookSeatsEmptyList) {
    std::vector<std::string> seatsToBook = {};
    bool success = service->BookSeats(1, 1, seatsToBook);
    EXPECT_FALSE(success);
}

TEST_F(BookingServiceTest, BookSeatsInvalidMovie) {
    std::vector<std::string> seatsToBook = {"a1"};
    EXPECT_FALSE(service->BookSeats(1, 9999, seatsToBook));
}

TEST_F(BookingServiceTest, BookSeatsInvalidTheater) {
    std::vector<std::string> seatsToBook = {"a1"};
    EXPECT_FALSE(service->BookSeats(9999, 1, seatsToBook));
}

TEST_F(BookingServiceTest, GetTheaterValid) {
    auto theater = store->GetTheater(1);
    ASSERT_TRUE(theater.has_value());
    EXPECT_EQ(theater->id, 1);
    EXPECT_EQ(theater->name, "Cinema City");
}

TEST_F(BookingServiceTest, GetTheaterInvalid) {
    auto theater = store->GetTheater(9999);
    EXPECT_FALSE(theater.has_value());
}

TEST_F(BookingServiceTest, CompleteBookingFlow) {
    auto movies = service->GetMovies();
    ASSERT_FALSE(movies.empty());

    auto theaters = service->GetTheaters(movies[0].id);
    ASSERT_FALSE(theaters.empty());

    auto seats = service->GetSeats(theaters[0].id, movies[0].id);
    ASSERT_FALSE(seats.empty());

    std::vector<std::string> seatsToBook = {seats[0].id};
    bool success = service->BookSeats(theaters[0].id, movies[0].id, seatsToBook);
    EXPECT_TRUE(success);

    auto updatedSeats = service->GetSeats(theaters[0].id, movies[0].id);
    EXPECT_TRUE(updatedSeats[0].isBooked);
}

// Try to book the same seat from multiple threads
// Only one should succeed
TEST_F(BookingServiceTest, ConcurrencyTest) {
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&]() {
            if (service->BookSeats(1, 1, {"a5"})) {
                successCount++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount, 1);
}

// Multiple threads booking different seats should all succeed
TEST_F(BookingServiceTest, ConcurrencyTestDifferentSeats) {
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;

    for (int i = 1; i <= 10; ++i) {
        threads.emplace_back([&, i]() {
            std::string seatId = "a" + std::to_string(i + 5);
            if (service->BookSeats(1, 1, {seatId})) {
                successCount++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount, 10);
}

// Book seats in different shows simultaneously
TEST_F(BookingServiceTest, MultipleShowsBooking) {
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;

    threads.emplace_back([&]() {
        if (service->BookSeats(1, 1, {"a1"})) {
            successCount++;
        }
    });

    threads.emplace_back([&]() {
        if (service->BookSeats(1, 2, {"a1"})) {
            successCount++;
        }
    });

    threads.emplace_back([&]() {
        if (service->BookSeats(2, 1, {"a1"})) {
            successCount++;
        }
    });

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount, 3);
}

// Stress Test
// Create a scenario with 100 threads trying to book 1000 seats total
// Theater 3 has 30 seats, so we'll use multiple theaters
TEST_F(BookingServiceTest, StressTest) {
    const int NUM_THREADS = 100;
    const int SEATS_PER_THREAD = 10;

    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&, i]() {
            std::vector<std::string> seatsToBook;
            for (int j = 0; j < SEATS_PER_THREAD; ++j) {
                int seatNum = (i * SEATS_PER_THREAD + j) % 20 + 1;
                seatsToBook.push_back("a" + std::to_string(seatNum));
            }

            if (service->BookSeats(1, 1, seatsToBook)) {
                successCount++;
            }
            else {
                failureCount++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount + failureCount, NUM_THREADS);
    EXPECT_GT(successCount, 0);

    auto seats = service->GetSeats(1, 1);
    int bookedCount = 0;
    for (const auto& seat : seats) {
        if (seat.isBooked) {
            bookedCount++;
        }
    }

    EXPECT_EQ(bookedCount, 20);
}
