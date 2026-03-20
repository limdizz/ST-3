// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <chrono>
#include <stdexcept>
#include <thread>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor &door) : door(door) {}

void DoorTimerAdapter::Timeout() {
  Timer timer;
  timer.tregister(door.getTimeOut(), nullptr);
  door.throwState();
}

TimedDoor::TimedDoor(int timeout)
    : adapter(new DoorTimerAdapter(*this)), iTimeout(timeout), isOpened(false) {
}

bool TimedDoor::isDoorOpened() { return isOpened; }

void TimedDoor::unlock() {
  isOpened = true;
  adapter->Timeout();
}

void TimedDoor::lock() { isOpened = false; }

int TimedDoor::getTimeOut() const { return iTimeout; }

void TimedDoor::throwState() {
  if (isOpened) {
    throw std::runtime_error("дверь все еще открыта");
  }
}

void Timer::sleep(int timeout) {
  if (timeout > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
  }
}

void Timer::tregister(int timeout, TimerClient *cl) {
  client = cl;
  sleep(timeout);
  if (client != nullptr) {
    client->Timeout();
  }
}
