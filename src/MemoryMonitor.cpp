#include "../include/MemoryMonitor.h"
#include "../include/SysMon.h" // Needed for SysMon::getInfo
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> // For std::remove

// Constructeur
MemoryMonitor::MemoryMonitor() {
    RAM.usage = 0.0f; //
    RAM.freeMem = 0.0f; //
    RAM.usageSwp = 0.0f; //
    RAM.freeSwp = 0.0f; //
    RAM.totalMemInMb = 0; //
    RAM.SwapMeminMb = 0; //
    update(); // Initial update to populate memory information
}

// Destructeur
MemoryMonitor::~MemoryMonitor() {
    // No dynamic memory to free in this class
}

bool MemoryMonitor::update() {
    memInfo.clear(); // Clear previous data
    std::string content = SysMon::getInfo("/proc/meminfo");
    if (content == " ") { // Handle error from getInfo
        std::cerr << "Error: Could not read /proc/meminfo." << std::endl;
        return false;
    }

    std::istringstream iss(content);
    std::string line;

    unsigned long long totalMem = 0;
    unsigned long long freeMem = 0;
    unsigned long long buffers = 0;
    unsigned long long cached = 0;
    unsigned long long totalSwap = 0;
    unsigned long long freeSwap = 0;
    unsigned long long sReclaimable = 0; // Added for more accurate free memory calculation

    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string key;
        long long value;
        std::string unit;

        lineStream >> key >> value >> unit;

        if (key == "MemTotal:") {
            totalMem = value;
        } else if (key == "MemFree:") {
            freeMem = value;
        } else if (key == "Buffers:") {
            buffers = value;
        } else if (key == "Cached:") {
            cached = value;
        } else if (key == "SwapTotal:") {
            totalSwap = value;
        } else if (key == "SwapFree:") {
            freeSwap = value;
        } else if (key == "SReclaimable:") { // For more accurate 'available' memory
            sReclaimable = value;
        }
    }

    // Modern kernels provide MemAvailable, which is a better indicator of truly free memory
    // If MemAvailable is not present, estimate it.
    unsigned long long memAvailable = 0;
    std::istringstream issAvailable(content);
    while (std::getline(issAvailable, line)) {
        if (line.find("MemAvailable:") != std::string::npos) {
            std::string valueStr = line.substr(line.find(":") + 1);
            std::istringstream valIss(valueStr);
            valIss >> memAvailable;
            break;
        }
    }

    if (memAvailable == 0) { // Fallback if MemAvailable is not reported
        memAvailable = freeMem + buffers + cached + sReclaimable;
    }

    RAM.totalMemInMb = totalMem / 1024; // KB to MB
    RAM.freeMem = static_cast<float>(memAvailable) / 1024.0f; // KB to MB (available memory)

    // Calculate actual used memory based on total and available
    unsigned long long usedMemKb = totalMem - memAvailable;
    RAM.usage = (totalMem > 0) ? (static_cast<float>(usedMemKb) / totalMem) * 100.0f : 0.0f;

    RAM.SwapMeminMb = totalSwap / 1024; // KB to MB
    RAM.freeSwp = static_cast<float>(freeSwap) / 1024.0f; // KB to MB

    RAM.usageSwp = (totalSwap > 0) ? (static_cast<float>(totalSwap - freeSwap) / totalSwap) * 100.0f : 0.0f;

    return true;
}

unsigned long long MemoryMonitor::getTotalMemory() const {
    return RAM.totalMemInMb * 1024; // Return in KB for consistency
}

unsigned long long MemoryMonitor::getFreeMemory() const {
    return static_cast<unsigned long long>(RAM.freeMem * 1024.0f); // Return in KB
}

unsigned long long MemoryMonitor::getUsedMemory() const {
    return getTotalMemory() - getFreeMemory(); // Calculated in KB
}

double MemoryMonitor::getMemoryUsagePercentage() const {
    return static_cast<double>(RAM.usage);
}

unsigned long long MemoryMonitor::getTotalSwap() const {
    return RAM.SwapMeminMb * 1024; // Return in KB
}

unsigned long long MemoryMonitor::getFreeSwap() const {
    return static_cast<unsigned long long>(RAM.freeSwp * 1024.0f); // Return in KB
}

unsigned long long MemoryMonitor::getUsedSwap() const {
    return getTotalSwap() - getFreeSwap(); // Calculated in KB
}

double MemoryMonitor::getSwapUsagePercentage() const {
    return static_cast<double>(RAM.usageSwp);
}

// Getting memory usage - this function seems to be for displaying/logging
// Re-purposed to use the internal RAM struct and potentially log.
std::size_t MemoryMonitor::memUsage(int logger) {
    // Ensure data is updated before displaying/logging
    update();

    std::cout << "Free memory : " << static_cast<unsigned long long>(RAM.freeMem * 1024) << "kB"
              << " Available memory : " << static_cast<unsigned long long>(RAM.freeMem * 1024) << "kB" // freeMem is already available mem
              << " \x1b[41mMemory usage : " << getUsedMemory() << "kB (" << static_cast<int>(RAM.usage) << "%)\x1b[0m" << std::endl;
    
    if (logger == options::_NLOG) {
        std::stringstream out;
        out << "Memory Monitor - " << SysMon::getTime() << "\n";
        out << "Total RAM: " << getTotalMemory() / 1024 << " MB, "
            << "Used RAM: " << getUsedMemory() / 1024 << " MB, "
            << "Free RAM: " << getFreeMemory() / 1024 << " MB, "
            << "Usage: " << getMemoryUsagePercentage() << "%\n";
        out << "Total Swap: " << getTotalSwap() / 1024 << " MB, "
            << "Used Swap: " << getUsedSwap() / 1024 << " MB, "
            << "Free Swap: " << getFreeSwap() / 1024 << " MB, "
            << "Swap Usage: " << getSwapUsagePercentage() << "%\n";
        SysMon::log(out);
    }
    return getUsedMemory() / 1024; // Return used memory in MB
}