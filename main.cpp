#include "fcfs.h"
#include "sjf.h"
#include "srtn.h"
#include "rr.h"
#include "priority.h"
#include "mlfq.h"
#include "simulator.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include "testsets.h"
#include "hrrn.h"
#include <functional>

std::vector<Process> makeTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                 std::uint64_t burst, int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid;
    p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "P1", 0, 8, 3);
  add(2, "P2", 1, 4, 1);
  add(3, "P3", 2, 9, 4);
  add(4, "P4", 3, 5, 2);
  return procs;
}

std::vector<Process> makeIoTestSet() {
  std::vector<Process> procs;
  auto add = [&](int pid, const std::string& name,
                 std::uint64_t arrival, std::uint64_t burst,
                 int priority,
                 std::vector<IoBlock> io = {}) {
    Process p;
    p.pid = pid; p.name = name;
    p.arrivalTime = arrival;
    p.burstTime = burst;
    p.remainingTime = burst;
    p.priority = priority;
    p.dynamicPriority = priority;
    p.ioBlocks = std::move(io);
    procs.push_back(p);
  };
  add(1, "CPU1", 0, 20, 2);
  add(2, "IO1",  1,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(3, "IO2",  2,  2, 1, {{1, 5}, {1, 5}, {0, 0}});
  add(4, "CPU2", 3, 10, 3);
  return procs;
}

void printResult(const SimResult& r) {
  std::cout << std::left << std::setw(30) << r.algorithm
            << " | wait=" << std::setw(7) << std::fixed << std::setprecision(2) << r.avgWaiting
            << " | turn=" << std::setw(7) << r.avgTurnaround
            << " | resp=" << std::setw(7) << r.avgResponse
            << " | CPU=" << std::setw(6) << r.cpuUtilization << "%"
            << " | CS=" << std::setw(4) << r.contextSwitches
            << " | done=" << r.throughput
            << "\n";
}

void printGantt(const SimResult& r) {
  std::cout << "Gantt (" << r.algorithm << "):\n";
  for (auto& [pid, span] : r.gantt) {
    std::cout << "  [" << span.first << "-" << span.second << ") ";
    if (pid == -1) std::cout << "IDLE\n";
      else if (pid == -2) std::cout << "CS\n";
    else std::cout << "P" << pid << "\n";
  }
  std::cout << "\n";
}


std::vector<Process> makeHrrnTestSet() {
    std::vector<Process> procs;
    auto add = [&](int pid, const std::string& name,
                   std::uint64_t arrival, std::uint64_t burst, int priority) {
        Process p;
        p.pid = pid; p.name = name;
        p.arrivalTime = arrival;
        p.burstTime = burst;
        p.remainingTime = burst;
        p.priority = priority;
        p.dynamicPriority = priority;
        procs.push_back(p);
    };
    add(1, "A",  0, 4, 1);
    add(2, "L",  1, 6, 1);
    add(3, "S1", 4, 2, 1);
    add(4, "S2", 5, 1, 1);
    return procs;
}


std::vector<Process> makeConvoySet() {
    std::vector<Process> procs;
    auto add = [&](int pid, const std::string& name, std::uint64_t arrival,
                   std::uint64_t burst, int priority,
                   std::vector<IoBlock> io = {}) {
        Process p;
        p.pid = pid; p.name = name;
        p.arrivalTime = arrival;
        p.burstTime = burst;
        p.remainingTime = burst;
        p.priority = priority;
        p.dynamicPriority = priority;
        p.ioBlocks = std::move(io);
        procs.push_back(p);
    };
    add(1, "CPU1", 0, 20, 2);
    add(2, "IO1",  1,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
    add(3, "IO2",  2,  6, 1, {{1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4}});
    add(4, "CPU2", 3, 12, 3);
    return procs;
}


using Factory = std::function<std::unique_ptr<Scheduler>(const std::vector<Process>&)>;

template <class T, class... Args>
Factory factory(Args... args) {
    return [=](const std::vector<Process>& s) { return std::make_unique<T>(s, args...); };
}

void runExperiment10() {
    std::vector<std::pair<std::string, Factory>> algos = {
        {"FCFS", factory<FcfsScheduler>()},
        {"SJF", factory<SjfScheduler>()},
        {"SRTN", factory<SrtnScheduler>()},
        {"HRRN", factory<HrrnScheduler>()},
        {"RR q=4", factory<RrScheduler>(4)},
        {"Prio", factory<PriorityScheduler>(false, false)},
        {"Prio+aging", factory<PriorityScheduler>(true, true)},
        {"MLFQ", factory<MlfqScheduler>()},
    };
    const int sizes[5] = {12, 14, 16, 18, 20};

    auto printTable = [&](const std::string& title,
                          std::function<double(const SimResult&)> metric) {
        std::cout << "\n" << title << "\n";
        std::cout << std::left << std::setw(12) << "Algorithm" << std::right;
        for (int i = 1; i <= 5; ++i) std::cout << std::setw(9) << ("set " + std::to_string(i));
        std::cout << std::setw(9) << "average" << "\n";
        for (auto& [label, make] : algos) {
            std::cout << std::left << std::setw(12) << label << std::right;
            double sum = 0;
            for (unsigned seed = 1; seed <= 5; ++seed) {
                auto set = makeRandomSet(seed, sizes[seed - 1]);
                auto sched = make(set);
                SimResult r = runSimulation(*sched);
                double v = metric(r);
                sum += v;
                std::cout << std::setw(9) << std::fixed << std::setprecision(2) << v;
            }
            std::cout << std::setw(9) << sum / 5 << "\n";
        }
    };

    printTable("Average waiting time", [](const SimResult& r) { return r.avgWaiting; });
    printTable("Average turnaround time", [](const SimResult& r) { return r.avgTurnaround; });
    printTable("Average response time", [](const SimResult& r) { return r.avgResponse; });
    printTable("CPU utilization %", [](const SimResult& r) { return r.cpuUtilization; });
}

int main() {
  std::cout << "Case 1: CPU-bound\n";
  {
    auto set1 = makeTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set1));
    scheds.push_back(std::make_unique<SjfScheduler>(set1));
    scheds.push_back(std::make_unique<SrtnScheduler>(set1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 1));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 2));
    scheds.push_back(std::make_unique<RrScheduler>(set1, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, false, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, false));
    scheds.push_back(std::make_unique<PriorityScheduler>(set1, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set1));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\nCase 2. Mixed loading: I/O + CPU\n";
  {
    auto set2 = makeIoTestSet();
    std::vector<std::unique_ptr<Scheduler>> scheds;
    scheds.push_back(std::make_unique<FcfsScheduler>(set2));
    scheds.push_back(std::make_unique<SrtnScheduler>(set2));
    scheds.push_back(std::make_unique<RrScheduler>(set2, 4));
    scheds.push_back(std::make_unique<PriorityScheduler>(set2, true, true));
    scheds.push_back(std::make_unique<MlfqScheduler>(set2));
    for (auto& s : scheds) printResult(runSimulation(*s));
  }

  std::cout << "\n";
  {
    auto set = makeTestSet();
    RrScheduler rr(set, 2);
    SimResult r = runSimulation(rr);
    printGantt(r);
  }

  
    {
        auto set = makeTestSet();
        RrScheduler rr(set, 2);
        SimResult r = runSimulation(rr);
        std::cout << "\nПо процессам, RR (q=2):\n";
        printProcessTable(rr.processes());
    }

    
    {
        auto set = makeTestSet();
        saveSet("sets/set_basic.txt", set);
        std::vector<Process> loaded;
        if (!loadSet("sets/set_basic.txt", loaded)) {
            std::cerr << "Не удалось загрузить sets/set_basic.txt\n";
        } else {
            FcfsScheduler a(set), b(loaded);
            std::cout << "\nСравнение FCFS: исходный набор vs загруженный\n";
            std::cout << "Исходный:    ";
            printResult(runSimulation(a));
            std::cout << "Загруженный: ";
            printResult(runSimulation(b));
        }
    }

    
    {
        auto set = makeHrrnTestSet();
        std::cout << "\n=== Сравнение SJF и HRRN на наборе из 4 процессов ===\n";
        SjfScheduler sjf(set);
        SimResult r1 = runSimulation(sjf);
        std::cout << "SJF:  ";
        printResult(r1);
        printGantt(r1);

        HrrnScheduler hrrn(set);
        SimResult r2 = runSimulation(hrrn);
        std::cout << "HRRN: ";
        printResult(r2);
        printGantt(r2);
    }

    
    {
        auto set = makeTestSet();
        std::cout << "\n=== Влияние switchCost на RR ===\n";
        std::cout << "q | wait(0) turn(0) | wait(1) turn(1) | CPU%(1) ovh%(1)\n";
        for (std::uint64_t q : {1, 2, 4, 8}) {
            RrScheduler rr0(set, q);
            SimResult r0 = runSimulation(rr0, 100000, 0);
            RrScheduler rr1(set, q);
            SimResult r1 = runSimulation(rr1, 100000, 1);
            std::cout << q << " | "
                      << std::fixed << std::setprecision(2)
                      << r0.avgWaiting << " " << r0.avgTurnaround << " | "
                      << r1.avgWaiting << " " << r1.avgTurnaround << " | "
                      << r1.cpuUtilization << " " << r1.overheadPercent << "\n";
        }
    }

    
    {
        auto set = makeTestSet();
        std::cout << "\nВлияние кванта в RR (набор makeTestSet)\n";
        std::cout << "q wait turn resp CS\n";
        for (std::uint64_t q : {1, 2, 4, 8, 16}) {
            RrScheduler rr(set, q);
            SimResult r = runSimulation(rr);
            std::cout << std::setw(2) << q << " " << std::fixed << std::setprecision(2)
                      << r.avgWaiting << " " << r.avgTurnaround << " " << r.avgResponse
                      << " " << std::setw(3) << r.contextSwitches << " "
                      << std::string(r.contextSwitches, '#') << "\n";
        }
    }

    
    {
        auto set = makeConvoySet();
        std::cout << "\n=== Эффект конвоя ===\n";

        std::cout << "\n--- FCFS ---\n";
        FcfsScheduler fcfs(set);
        SimResult rf = runSimulation(fcfs);
        printResult(rf);
        printGantt(rf);
        printProcessTable(fcfs.processes());

        std::cout << "\n--- SRTN ---\n";
        SrtnScheduler srtn(set);
        SimResult rs = runSimulation(srtn);
        printResult(rs);

        std::cout << "\n--- RR (q=4) ---\n";
        RrScheduler rr(set, 4);
        SimResult rr4 = runSimulation(rr);
        printResult(rr4);

        std::cout << "\n--- Priority (preemptive) + aging ---\n";
        PriorityScheduler prio(set, true, true);
        SimResult rp = runSimulation(prio);
        printResult(rp);

        std::cout << "\n--- MLFQ ---\n";
        MlfqScheduler mlfq(set);
        SimResult rm = runSimulation(mlfq);
        printResult(rm);
        printGantt(rm);
        printProcessTable(mlfq.processes());
    }

    
    {
        const int sizes[5] = {12, 14, 16, 18, 20};
        for (unsigned seed = 1; seed <= 5; ++seed) {
            saveSet("sets/set" + std::to_string(seed) + ".txt",
                    makeRandomSet(seed, sizes[seed - 1]));
        }
        std::cout << "\n5 наборов сохранены в sets/\n";
    }

        runExperiment10();

    return 0;
}
