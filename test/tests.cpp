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

TEST_F(TimedDoorFixture, TestIsDoorOpened) {
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, TestIsDoorOpenedAfterLock) {
  door->lock();
  EXPECT_FALSE(door->isDoorOpened());
}

TEST_F(TimedDoorFixture, TestNoThrowForClosedDoor) {
  EXPECT_NO_THROW(door->throwState());
}

TEST_F(TimedDoorFixture, TestThrowsForOpenedDoor) {
  EXPECT_THROW(door->unlock(), std::runtime_error);
  EXPECT_THROW(door->throwState(), std::runtime_error);
}

TEST(TimedDoor, TestUnlockThrowsIfDoorIsOpenedAfterTimeout) {
  TimedDoor door(0);
  door.lock();
  EXPECT_THROW(door.unlock(), std::runtime_error);
}

TEST(TimedDoor, TestTimeoutValueIsStoredInDoor) {
  TimedDoor door(123);
  EXPECT_EQ(door.getTimeOut(), 123);
}

TEST(TimedDoor, TestDoorIsOpenedAfterUnlockException) {
  TimedDoor door(0);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(TimedDoor, TestUnlockNotThrowIfDoorGetsClosedBeforeTimeout) {
  TimedDoor door(80);
  door.lock();

  auto worker = std::async(std::launch::async, [&door]() -> bool {
    EXPECT_NO_THROW(door.unlock());
    return false;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  door.lock();

  worker.wait();
  EXPECT_FALSE(door.isDoorOpened());
}

TEST(Timer, TestRegisterWithNullClientDoesNotThrow) {
  Timer timer;
  EXPECT_NO_THROW(timer.tregister(0, nullptr));
}

TEST(Timer, TestRegisterCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(0, &client);
}

TEST(Timer, TestRegisterWithDelayCallsClientTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(2, &client);
}

TEST(DoorTimerAdapter, TestTimeoutThrowsWhenDoorIsOpened) {
  TimedDoor door(0);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);

  DoorTimerAdapter adapter(door);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(DoorTimerAdapter, TestTimeoutDoesNotThrowWhenDoorIsClosed) {
  TimedDoor door(0);
  door.lock();
  DoorTimerAdapter adapter(door);
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(Timer, TestRegisterWithNegativeTimeout) {
  Timer timer;
  MockTimerClient client;
  EXPECT_CALL(client, Timeout()).Times(1);
  timer.tregister(-5, &client);
}

TEST(Timer, TestTimerStoresClientCorrectly) {
  Timer timer;
  MockTimerClient client1;
  MockTimerClient client2;

  EXPECT_CALL(client1, Timeout()).Times(1);
  timer.tregister(0, &client1);

  EXPECT_CALL(client2, Timeout()).Times(1);
  timer.tregister(0, &client2);
}

TEST(DoorTimerAdapter, TestTimeoutWithDifferentTimeoutValues) {
  TimedDoor door(50);
  door.lock();
  DoorTimerAdapter adapter(door);

  EXPECT_NO_THROW(adapter.Timeout());

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_THROW(adapter.Timeout(), std::runtime_error);
}

TEST(TimedDoor, TestDoorCanBeReopenedAfterClosing) {
  TimedDoor door(50);
  door.lock();

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_TRUE(door.isDoorOpened());

  door.lock();
  EXPECT_FALSE(door.isDoorOpened());

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(DoorTimerAdapter, TestMultipleAdaptersForSameDoor) {
  TimedDoor door(0);
  door.lock();

  DoorTimerAdapter adapter1(door);
  DoorTimerAdapter adapter2(door);

  EXPECT_NO_THROW(adapter1.Timeout());
  EXPECT_NO_THROW(adapter2.Timeout());

  EXPECT_THROW(door.unlock(), std::runtime_error);

  EXPECT_THROW(adapter1.Timeout(), std::runtime_error);
  EXPECT_THROW(adapter2.Timeout(), std::runtime_error);
}

TEST(TimedDoor, TestDoorStateAfterMultipleUnlockAttempts) {
  TimedDoor door(0);
  door.lock();

  for (int i = 0; i < 5; ++i) {
    EXPECT_THROW(door.unlock(), std::runtime_error);
    EXPECT_TRUE(door.isDoorOpened());
    door.lock();
    EXPECT_FALSE(door.isDoorOpened());
  }
}

TEST(DoorTimerAdapter, TestTimeoutAfterDoorClosed) {
  TimedDoor door(50);
  door.lock();
  DoorTimerAdapter adapter(door);

  EXPECT_THROW(door.unlock(), std::runtime_error);

  door.lock();
  EXPECT_NO_THROW(adapter.Timeout());
}

TEST(TimedDoor, TestDoorWithMaxTimeout) {
  const int MAX_TIMEOUT = 10000;
  TimedDoor door(MAX_TIMEOUT);
  door.lock();

  EXPECT_EQ(door.getTimeOut(), MAX_TIMEOUT);
  EXPECT_FALSE(door.isDoorOpened());

  EXPECT_THROW(door.unlock(), std::runtime_error);
  EXPECT_TRUE(door.isDoorOpened());
}

TEST(TimedDoor, TestSafeDeletion) {
  TimedDoor* dynamicDoor = new TimedDoor(10);

  EXPECT_THROW(dynamicDoor->unlock(), std::runtime_error);

  EXPECT_NO_THROW({
    delete dynamicDoor;
  });
}

TEST(Timer, TestOverrideClientImplicitly) {
  Timer timer;
  MockTimerClient client1;
  MockTimerClient client2;

  EXPECT_CALL(client1, Timeout()).Times(1);
  timer.tregister(0, &client1);

  EXPECT_CALL(client2, Timeout()).Times(1);
  timer.tregister(0, &client2);
}

TEST(TimedDoor, TestZeroTimeoutThrowsImmediately) {
  TimedDoor zeroDoor(0);
  EXPECT_THROW(zeroDoor.unlock(), std::runtime_error);
}
