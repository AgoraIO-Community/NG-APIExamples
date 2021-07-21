// ======================= TestUitls.h ======================= 
#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

#include "AgoraRteBase.h"

// Warning, this is just for test purpose only semaphore since Wait()
// is not accurate
//
class agora_test_semaphore {
 public:
  agora_test_semaphore(int count = 0) : left_count_(count) {}

  int GetCount() { return left_count_; }

  inline void Notify() {
    bool do_notify = false;

    {
      std::unique_lock<std::mutex> guard_lock(lock_);

      if (left_count_ == 0 && wait_count_) {
        do_notify = true;
      }

      left_count_++;
    }

    if (do_notify) {
      cv_.notify_one();
    }
  }

  // considering spurious wakeup, For test purpose , we don't need a very
  // accurate wait timeout.
  //
  inline int Wait(int seconds) {
    int time_left = seconds;
    bool is_forever_wait = false;

    if (time_left < 0) is_forever_wait = true;

    auto time_now = std::chrono::system_clock::now();
    bool is_wait_added = false;

    std::unique_lock<std::mutex> guard_lock(lock_);

    while (left_count_ == 0) {
      if (!is_forever_wait && time_left <= 0) {
        if (is_wait_added) wait_count_--;

        return -1;
      }

      if (!is_wait_added) {
        is_wait_added = true;
        wait_count_++;
      }

      if (is_forever_wait) {
        cv_.wait(guard_lock);
      } else {
        cv_.wait_for(guard_lock, std::chrono::seconds(time_left));

        auto wait_out = std::chrono::system_clock::now();
        auto wait_seconds = std::chrono::duration_cast<std::chrono::seconds>(
            wait_out - time_now);
        time_now = wait_out;
        time_left -= static_cast<int>(wait_seconds.count());
      }
    }

    left_count_--;

    if (is_wait_added) {
      wait_count_--;
    }

    return 1;
  }

 private:
  std::mutex lock_;
  std::condition_variable cv_;
  int wait_count_ = 0;
  int left_count_ = 0;
};

struct TestVideoPacket {
  TestVideoPacket()
      : data(nullptr),
        size(0),
        flags(0),
        timestamp(0),
        rotation(agora::rte::VIDEO_ORIENTATION::VIDEO_ORIENTATION_0) {}
  uint8_t* data;
  int size;
  int flags;
  int64_t timestamp;  // ms
  agora::rte::VIDEO_ORIENTATION rotation;
};

class RteTestGloablSettings {
 public:
  static std::string GetAppId() { return "aab8b8f5a8cd4469a63042fcfafe7063"; }
};

