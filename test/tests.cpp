// Copyright 2021 GHA Test Team

#include "TimedDoor.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <stdexcept>
#include <thread>

class MockTimerClient : public TimerClient {
public:
  MOCK_METHOD(void, Timeout, (), (override));
};

class TimedDoorFixture : public ::testing::Test {
protected:
  std::unique_ptr<TimedDoor> door;

  void SetUp() override {
    door = std::make_unique<TimedDoor>(10);
    door->lock();
  }

  void TearDown() override { door.reset(); }
};

TEST_F(TimedDoorFixture, CheckDoorIsClosed) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, CheckLockAndDoorClosed) {
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, CheckNoThrowForClosedDoor) {
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorFixture, CheckThrowsForOpenedDoor) {
  try {
    door->unlock();
  } catch (const std::runtime_error &) {
  }
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST(TimedDoorStandalone, CheckUnlockThrowsIfDoorIsOpenAfterTimeout) {
  TimedDoor door(0);
  door.lock();
  EXPECT_THROW(door.unlock(), std::runtime_error);
}

TEST(TimedDoorStandalone, CheckTimeoutValueIsStoredInDoor) {
  TimedDoor door(123);
  EXPECT_EQ(door.getTimeOut(), 123);
}

TEST(TimedDoorStandalone, CheckDoorRemainsOpenedAfterUnlockException) {
  TimedDoor door(0);
  door.lock();
  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(TimedDoorStandalone, CheckUnlockNotThrowIfDoorGetsClosedBeforeTimeout) {
  TimedDoor door(80);
  door.lock();

  auto worker = std::async(std::launch::async, [&door]() -> bool {
    try {
      door.unlock();
      return false;
    } catch (const std::runtime_error &) {
      return true;
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  door.lock();

  EXPECT_FALSE(worker.get());
  EXPECT_FALSE(door.isDoorOpened());
}

TEST(TimerStandalone, CheckRegisterWithNullClientDoesNotThrow) {
  Timer timer;
  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST(TimerStandalone, CheckRegisterCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(TimerStandalone, CheckRegisterWithDelayCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(2, &client);
}

TEST(DoorTimerAdapterStandalone, CheckTimeoutThrowsWhenDoorOpened) {
  TimedDoor door(0);
  door.lock();
  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }

  DoorTimerAdapter adapter(door);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(DoorTimerAdapterStandalone, CheckTimeoutDoesNotThrowWhenDoorClosed) {
  TimedDoor door(0);
  door.lock();
  DoorTimerAdapter adapter(door);
  EXPECT_NO_THROW(adapter.Timeout());
}
