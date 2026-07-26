#include "cpu.h"

#include <fstream>
#include <sstream>

Cpu::Cpu() : initialized_(false) {}

Cpu::CpuTimes Cpu::getCpuTimes() {
    std::ifstream file("/proc/stat");

    CpuTimes result{0, 0};

    if (!file.is_open()) return result;

    std::string line;
    std::getline(file, line);

    std::istringstream ss(line);

    std::string cpu;
    long long user = 0;
    long long nice = 0;
    long long system = 0;
    long long idle = 0;
    long long iowait = 0;
    long long irq = 0;
    long long softirq = 0;
    long long steal = 0;

    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    result.idle = idle + iowait;

    result.total = user + nice + system + idle + iowait + irq + softirq + steal;

    return result;
}

double Cpu::usage() {
    CpuTimes current = getCpuTimes();

    if (!initialized_) {
        previous_ = current;
        initialized_ = true;
        return 0.0;
    }

    long long totalDelta = current.total - previous_.total;

    long long idleDelta = current.idle - previous_.idle;

    previous_ = current;

    if (totalDelta <= 0) return 0.0;

    return 100.0 * static_cast<double>(totalDelta - idleDelta) / static_cast<double>(totalDelta);
}