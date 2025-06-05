#include "../include/CpuMonitor.h"
#include "../include/SysMon.h" // Needed for SysMon::getInfo
#include <fstream>
#include <sstream>
#include <iostream> // For cerr

// Constructeur
CpuMonitor::CpuMonitor() {
    CPU.frequencyMax = 0; // Will attempt to get this from /proc/cpuinfo if possible
    CPU.nbrCPU = 0; // Will be determined from /proc/stat
    CPU.usageCPU = 0.0f;
    CPU.usagePerCPU = nullptr; // Initialize to nullptr
    updateTimes(); // Get initial CPU times
}

// Destructeur
CpuMonitor::~CpuMonitor() {
    if (CPU.usagePerCPU != nullptr) {
        delete[] CPU.usagePerCPU;
        CPU.usagePerCPU = nullptr;
    }
}

// Reads CPU time values from /proc/stat
CpuTimes CpuMonitor::readCpuTimes() {
    CpuTimes times;
    std::string line = SysMon::getInfo("/proc/stat");
    if (line == " ") { // Handle error from getInfo
        std::cerr << "Error: Could not read /proc/stat for CPU times." << std::endl;
        return times; // Return default initialized times
    }

    std::istringstream iss(line);
    std::string cpuName;
    iss >> cpuName; // Read "cpu"

    // Read global CPU times
    iss >> times.user >> times.nice >> times.system >> times.idle >> times.iowait
        >> times.irq >> times.softirq >> times.steal >> times.guest >> times.guest_nice;
    
    // Count number of CPUs and initialize usagePerCPU
    // We'll re-read /proc/stat to count "cpuX" lines
    std::string fullStatContent = SysMon::getInfo("/proc/stat");
    std::istringstream fullIss(fullStatContent);
    std::string currentLine;
    short tempNbrCPU = 0;
    while (std::getline(fullIss, currentLine)) {
        if (currentLine.rfind("cpu", 0) == 0 && currentLine.length() > 3 && std::isdigit(currentLine[3])) {
            tempNbrCPU++;
        }
    }
    
    if (CPU.nbrCPU != tempNbrCPU) {
        CPU.nbrCPU = tempNbrCPU;
        if (CPU.usagePerCPU != nullptr) {
            delete[] CPU.usagePerCPU;
        }
        CPU.usagePerCPU = new float[CPU.nbrCPU];
        for (int i = 0; i < CPU.nbrCPU; ++i) {
            CPU.usagePerCPU[i] = 0.0f;
        }
    }

    return times;
}

void CpuMonitor::updateTimes() {
    previousTimes = currentTimes; // Save current as previous
    currentTimes = readCpuTimes(); // Get new current times
}

float CpuMonitor::getCpuUsage() {
    updateTimes(); // Always update before calculating usage

    long long prevIdle = previousTimes.totalIdleTime();
    long long prevTotal = previousTimes.totalTime();
    long long currIdle = currentTimes.totalIdleTime();
    long long currTotal = currentTimes.totalTime();

    long long totalDiff = currTotal - prevTotal;
    long long idleDiff = currIdle - prevIdle;

    if (totalDiff == 0) {
        return 0.0f; // Avoid division by zero
    }

    CPU.usageCPU = 100.0f * (1.0f - static_cast<float>(idleDiff) / totalDiff);

    // To-Do: Calculate usage per CPU if needed, requires parsing each "cpuX" line
    // For now, usagePerCPU will remain 0.0f unless implemented
    return CPU.usageCPU;
}

float CpuMonitor::getCpuFreq() {
    // Reading CPU frequency can be tricky as it changes dynamically.
    // /proc/cpuinfo provides static info, actual frequency is often in /sys/devices/system/cpu/cpuX/cpufreq/scaling_cur_freq
    // This requires reading multiple files. For simplicity, let's use a placeholder or read base frequency from /proc/cpuinfo.

    std::string cpuinfoContent = SysMon::getInfo("/proc/cpuinfo");
    std::istringstream iss(cpuinfoContent);
    std::string line;
    float currentFreq = 0.0f;
    while (std::getline(iss, line)) {
        if (line.find("cpu MHz") != std::string::npos) {
            std::string freqStr = line.substr(line.find(":") + 1);
            try {
                currentFreq = std::stof(freqStr);
                // Assuming all cores have roughly the same frequency for simplicity
                break; 
            } catch (const std::exception& e) {
                // Handle parsing error
            }
        }
    }
    CPU.frequency = currentFreq;

    // Get max frequency from /proc/cpuinfo, typically 'cpu MHz' on the first processor entry
    if (CPU.frequencyMax == 0) { // Only set once
        std::istringstream maxFreqIss(cpuinfoContent);
        while (std::getline(maxFreqIss, line)) {
            if (line.find("MHz") != std::string::npos) {
                std::string freqStr = line.substr(line.find(":") + 1);
                try {
                    CPU.frequencyMax = std::stof(freqStr);
                    break;
                } catch (const std::exception& e) {
                    // Handle parsing error
                }
            }
        }
    }
    return CPU.frequency;
}

std::string CpuMonitor::getCpuInfo() {
    // This can return raw content of /proc/cpuinfo or a formatted string.
    // For now, let's return a summary.
    std::string info = SysMon::getInfo("/proc/cpuinfo");
    std::stringstream ss;
    std::string line;
    std::string modelName = "N/A";
    int cores = 0;

    std::istringstream iss(info);
    while (std::getline(iss, line)) {
        if (line.find("model name") != std::string::npos) {
            modelName = line.substr(line.find(":") + 2);
            // Remove leading/trailing whitespace
            modelName.erase(0, modelName.find_first_not_of(" \t"));
            modelName.erase(modelName.find_last_not_of(" \t\r\n") + 1);
        } else if (line.find("cpu cores") != std::string::npos) {
            std::string coresStr = line.substr(line.find(":") + 2);
            try {
                cores = std::stoi(coresStr);
            } catch (const std::exception& e) {
                // Handle parsing error
            }
        }
    }
    
    ss << "CPU Model: " << modelName << "\n";
    ss << "CPU Cores: " << cores << "\n";
    ss << "Current Freq: " << CPU.frequency << " MHz\n";
    ss << "Max Freq: " << CPU.frequencyMax << " MHz\n";

    rawCPU = ss.str(); // Store the formatted info
    return rawCPU;
}


bool CpuMonitor::update() {
    getCpuUsage(); // This also updates currentTimes and previousTimes
    getCpuFreq();
    // getCpuInfo(); // No need to call this on every update unless the model changes
    return true;
}