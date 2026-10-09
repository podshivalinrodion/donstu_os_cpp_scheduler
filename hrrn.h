#pragma once

#include "scheduler.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// Highest Response Ratio Next (HRRN)
// Невытесняющий. Выбирает процесс с наибольшим коэффициентом отклика:
//     R = (waitingTime + burstTime) / burstTime
class HrrnScheduler : public Scheduler {
public:
  explicit HrrnScheduler(const std::vector<Process>& procs)
      : Scheduler(procs) {}

  std::string name() const override { return "HRRN"; }

  void onTick(std::uint64_t tick) override {
    for (auto& p : processes()) {
      if (p.state == ProcessState::NEW && p.arrivalTime <= tick) {
        p.state = ProcessState::READY;
        ready_.push_back(p.pid);
        onProcessReady(p.pid, tick);
      }
    }
  }

  int pickNext(std::uint64_t) override {
    if (ready_.empty()) return -1;

    auto ratio = [&](int pid) {
      const Process* p = find(pid);
      // R = (waitingTime + burstTime) / burstTime
      return static_cast<double>(p->waitingTime + p->burstTime) /
             static_cast<double>(p->burstTime);
    };

    auto it = std::max_element(ready_.begin(), ready_.end(),
                               [&](int a, int b) {
                                 return ratio(a) < ratio(b);
                               });

    int pid = *it;
    ready_.erase(it);
    return pid;
  }

  void onProcessReady(int pid, std::uint64_t) override {
    // процесс уже добавлен в onTick
    (void)pid;
  }

  void onProcessFinished(int pid, std::uint64_t) override {
    Process* p = find(pid);
    if (p) p->state = ProcessState::TERMINATED;
    ready_.erase(std::remove(ready_.begin(), ready_.end(), pid),
                 ready_.end());
  }

  void onProcessBlocked(int /*pid*/, std::uint64_t) override {}

  void onProcessPreempted(int pid, std::uint64_t) override {
    ready_.push_back(pid);
  }

  bool shouldPreempt(int, std::uint64_t) override { return false; }

private:
  std::vector<int> ready_;
};
