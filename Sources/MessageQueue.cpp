//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//
#include <DebugServer2/MessageQueue.h>

#include <chrono>

using ds2::MessageQueue;

// FIXME(strager): This class does not handle spurious
// wakeups!  See http://en.wikipedia.org/wiki/Spurious_wakeup

MessageQueue::MessageQueue() {}

void MessageQueue::put(std::string const &message) {
  std::lock_guard<std::mutex> guard(_lock);
  _messages.push_back(message);
  _ready.notify_one();
}

std::string MessageQueue::get(int wait) {
  std::string message;

  std::unique_lock<std::mutex> lock(_lock);
  if (_messages.empty()) {
    if (wait < 0) {
      _ready.wait(lock);
    } else {
      if (_ready.wait_for(lock, std::chrono::milliseconds(wait)) ==
          std::cv_status::timeout)
        return std::string();
    }
  }
  if (!_messages.empty()) {
    message = _messages.front();
    _messages.pop_front();
  }

  return message;
}

bool MessageQueue::wait(int ms) {
  std::unique_lock<std::mutex> lock(_lock);
  if (!_messages.empty())
    return true;
  if (ms < 0) {
    _ready.wait(lock);
    return !_messages.empty();
  } else {
    std::chrono::milliseconds duration(ms);
    return _ready.wait_for(lock, duration) != std::cv_status::timeout;
  }
}

void MessageQueue::clear(bool signal) {
  std::lock_guard<std::mutex> guard(_lock);
  _messages.clear();
  if (signal) {
    _ready.notify_one();
  }
}
