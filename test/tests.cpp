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

TEST(TimedDoor, CheckUnlockThrowsIfDoorIsOpenAfterTimeout) {
  TimedDoor door(0);
  door.lock();
  EXPECT_THROW(door.unlock(), std::runtime_error);
}

TEST(TimedDoor, CheckTimeoutValueIsStoredInDoor) {
  TimedDoor door(123);
  EXPECT_EQ(door.getTimeOut(), 123);
}

TEST(TimedDoor, CheckDoorRemainsOpenedAfterUnlockException) {
  TimedDoor door(0);
  door.lock();
  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(TimedDoor, CheckUnlockNotThrowIfDoorGetsClosedBeforeTimeout) {
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

TEST(Timer, CheckRegisterWithNullClientDoesNotThrow) {
  Timer timer;
  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST(Timer, CheckRegisterCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(Timer, CheckRegisterWithDelayCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(2, &client);
}

TEST(DoorTimerAdapter, CheckTimeoutThrowsWhenDoorOpened) {
  TimedDoor door(0);
  door.lock();
  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }

  DoorTimerAdapter adapter(door);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(DoorTimerAdapter, CheckTimeoutDoesNotThrowWhenDoorClosed) {
  TimedDoor door(0);
  door.lock();
  DoorTimerAdapter adapter(door);
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(Timer, CheckRegisterWithNegativeTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(-5, &client);
}

TEST(Timer, CheckTimerStoresClientCorrectly) {
  Timer timer;
  MockTimerClient client1;
  MockTimerClient client2;

  EXPECT_CALL(client1, Timeout()).Times(1);
  timer.tregister(0, &client1);

  EXPECT_CALL(client2, Timeout()).Times(1);
  timer.tregister(0, &client2);
}

TEST(DoorTimerAdapter, CheckTimeoutWithDifferentTimeoutValues) {
  TimedDoor door(50);
  door.lock();
  DoorTimerAdapter adapter(door);

  EXPECT_NO_THROW(adapter.Timeout());

  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(TimedDoor, CheckDoorCanBeReopenedAfterClosing) {
  TimedDoor door(50);
  door.lock();

  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }
  EXPECT_TRUE(door.isDoorOpened());

  door.lock();
  EXPECT_FALSE(door.isDoorOpened());

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(DoorTimerAdapter, CheckMultipleAdaptersForSameDoor) {
  TimedDoor door(0);
  door.lock();

  DoorTimerAdapter adapter1(door);
  DoorTimerAdapter adapter2(door);

  EXPECT_NO_THROW(adapter1.Timeout());
  EXPECT_NO_THROW(adapter2.Timeout());

  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }

  EXPECT_THROW(adapter1.Timeout(), std::runtime_error);
  EXPECT_THROW(adapter2.Timeout(), std::runtime_error);
}

TEST(TimedDoor, CheckDoorStateAfterMultipleUnlockAttempts) {
  TimedDoor door(0);
  door.lock();

  for (int i = 0; i < 5; ++i) {
    try {
      door.unlock();
    } catch (const std::runtime_error &) {
    }
    EXPECT_TRUE(door.isDoorOpened());
    door.lock();
    EXPECT_FALSE(door.isDoorOpened());
  }
}

TEST(DoorTimerAdapter, CheckTimeoutAfterDoorClosed) {
  TimedDoor door(50);
  door.lock();
  DoorTimerAdapter adapter(door);

  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }

  door.lock();

  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(TimedDoor, CheckDoorWithMaxTimeout) {
  const int MAX_TIMEOUT = 10000;
  TimedDoor door(MAX_TIMEOUT);
  door.lock();

  EXPECT_EQ(door.getTimeOut(), MAX_TIMEOUT);
  EXPECT_FALSE(door.isDoorOpened());

  try {
    door.unlock();
  } catch (const std::runtime_error &) {
  }
  EXPECT_TRUE(door.isDoorOpened());
}
