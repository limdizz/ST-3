// Copyright 2021 GHA Test Team
#include "TimedDoor.h"

#include <chrono>
#include <stdexcept>
#include <thread>

DoorTimerAdapter::DoorTimerAdapter(TimedDoor &d) : door(d) {}

void DoorTimerAdapter::Timeout() { door.throwState(); }

TimedDoor::TimedDoor(int timeout)
    : adapter(new DoorTimerAdapter(*this)), iTimeout(timeout), isOpened(false) {
}

TimedDoor::~TimedDoor() { delete adapter; }

bool TimedDoor::isDoorOpened() { return isOpened; }

void TimedDoor::unlock() {
  isOpened = true;
  Timer timer;
  timer.tregister(iTimeout, adapter);
}

void TimedDoor::lock() { isOpened = false; }

int TimedDoor::getTimeOut() const { return iTimeout; }

void TimedDoor::throwState() {
  if (isOpened) {
    throw std::runtime_error("Дверь до сих пор открыта!");
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
