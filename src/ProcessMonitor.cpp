#include "../include/ProcessMonitor.h"
#include "../include/SysMon.h" // Needed for SysMon::getInfo, isNumber, getVector
#include <string>
#include <vector>
#include <iostream>
#include <filesystem> // For std::filesystem::directory_iterator
#include <algorithm>  // For std::remove_if
#include <unistd.h>   // For sysconf(_SC_PAGESIZE), sysconf(_SC_CLK_TCK)
#include <iomanip>    // For std::fixed, std::setprecision

using namespace std;

// Constructeur
ProcessMonitor::ProcessMonitor() {
    nbrProcess = 0; //
}

// Destructeur
ProcessMonitor::~ProcessMonitor() {
    // No dynamic memory specific to this class to free, vector handles its memory
}

bool ProcessMonitor::update() {
    activeProcessesList.clear(); // Clear previous list
    nbrProcess = 0; // Reset process count

    std::string path = "/proc"; //
    for (auto const &entry : std::filesystem::directory_iterator(path)) { //
        std::string newPath = entry.path(); //
        std::string pathFileName = entry.path().filename(); //

        if (isNumber(pathFileName)) { // Check if it's a PID directory
            std::string statusFilePath = newPath + "/status";
            std::string statFilePath = newPath + "/stat"; // For CPU usage and uptime
            
            std::string fileContent = SysMon::getInfo(statusFilePath); //
            if (fileContent == " ") { // If status file cannot be read (e.g., process died)
                continue;
            }

            std::string statContent = SysMon::getInfo(statFilePath);
            if (statContent == " ") {
                continue;
            }

            std::istringstream issStatus(fileContent); //
            std::istringstream issStat(statContent);

            activeProcesses proc;
            proc.pid = std::stoll(pathFileName); // Convert filename to PID

            // Parse /proc/[pid]/status for Name, Uid (for user), VmRSS (for memory)
            std::string line;
            while (std::getline(issStatus, line)) {
                if (line.rfind("Name:", 0) == 0) {
                    proc.pathName = line.substr(line.find(":") + 1);
                    proc.pathName.erase(0, proc.pathName.find_first_not_of(" \t")); // Trim leading whitespace
                } else if (line.rfind("Uid:", 0) == 0) {
                    std::istringstream uidIss(line.substr(line.find(":") + 1));
                    long uid;
                    uidIss >> uid;
                    // Get username from UID (requires system call or reading /etc/passwd)
                    // For simplicity, let's just store the UID string for now or map to a common user if feasible.
                    // This is a complex part for a simple monitor without external libs.
                    // For now, we'll just put the UID as a string or a placeholder
                    proc.user = std::to_string(uid); 
                } else if (line.rfind("VmRSS:", 0) == 0) {
                    std::string memStr = line.substr(line.find(":") + 1);
                    std::istringstream memIss(memStr);
                    long long vmRssKb;
                    memIss >> vmRssKb;
                    proc.memory = static_cast<float>(vmRssKb) / 1024.0f; // Convert KB to MB
                }
            }

            // Parse /proc/[pid]/stat for CPU usage and uptime
            // Fields: 14 utime, 15 stime, 22 starttime
            std::vector<std::string> statTokens;
            std::string token;
            while (issStat >> token) {
                statTokens.push_back(token);
            }

            if (statTokens.size() >= 22) {
                long long utime = std::stoll(statTokens[13]); // user time
                long long stime = std::stoll(statTokens[14]); // system time
                long long cutime = std::stoll(statTokens[15]); // children user time
                long long cstime = std::stoll(statTokens[16]); // children system time
                long long starttime = std::stoll(statTokens[21]); // start time in jiffies

                long long totalTime = utime + stime + cutime + cstime;

                // Calculate CPU percentage for this process (requires previous values)
                // This is a simplified calculation and usually needs more sophisticated tracking
                // over an interval for accurate per-process CPU usage.
                // For a quick snapshot, it's total_time_in_seconds / sys_uptime_in_seconds * 100
                long long sys_uptime_jiffies = 0;
                std::string uptimeContent = SysMon::getInfo("/proc/uptime");
                if (uptimeContent != " ") {
                    std::istringstream uptimeIss(uptimeContent);
                    double uptimeSeconds;
                    uptimeIss >> uptimeSeconds;
                    sys_uptime_jiffies = static_cast<long long>(uptimeSeconds * sysconf(_SC_CLK_TCK));
                }
                
                if (sys_uptime_jiffies > 0) {
                    proc.cpu = (static_cast<float>(totalTime) / sys_uptime_jiffies) * 100.0f;
                } else {
                    proc.cpu = 0.0f;
                }
                
                // Calculate process uptime
                if (sys_uptime_jiffies > 0) {
                    proc.uptime = (sys_uptime_jiffies - starttime) / sysconf(_SC_CLK_TCK); // Uptime in seconds
                } else {
                    proc.uptime = 0;
                }
            } else {
                proc.cpu = 0.0f;
                proc.uptime = 0;
            }
            
            activeProcessesList.push_back(proc);
            nbrProcess++; //
        }
    }
    return true;
}

// This method would display current process information
std::string ProcessMonitor::getProcessInfo() {
    std::stringstream ss;
    ss << "\x1b[42m" << std::left << std::setw(15) << "PID"
       << std::left << std::setw(30) << "NAME"
       << std::left << std::setw(10) << "CPU%"
       << std::left << std::setw(10) << "MEM(MB)"
       << std::left << std::setw(15) << "USER"
       << std::left << std::setw(15) << "UPTIME"
       << "\x1b[0m\n";

    for (const auto& proc : activeProcessesList) {
        ss << std::left << std::setw(15) << proc.pid
           << std::left << std::setw(30) << proc.pathName
           << std::left << std::setw(10) << std::fixed << std::setprecision(2) << proc.cpu
           << std::left << std::setw(10) << std::fixed << std::setprecision(2) << proc.memory
           << std::left << std::setw(15) << proc.user
           << std::left << std::setw(15) << proc.uptime << "s"
           << "\n";
    }
    ss << "Total Processes: " << nbrProcess << "\n";
    return ss.str();
}

std::string ProcessMonitor::getProcessRaw() {
    // This function is redundant with getProcessInfo if it's meant for display.
    // If it's for raw data for internal use, it should return a structured format.
    // For display, getProcessInfo is better. Let's make this return an empty string
    // or adapt it if there's a specific "raw" output requirement.
    // For now, it will simply call getProcessInfo for display.
    return getProcessInfo();
}

const std::vector<activeProcesses>& ProcessMonitor::getActiveProcesses() const {
    return activeProcessesList;
}