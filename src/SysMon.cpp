#include "../include/SysMon.h"
#include <unistd.h>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip> // For std::fixed, std::setprecision

using namespace std; //

// Constructor
SysMon::SysMon(int updateInterval, bool fullLog) //
    : CpuMonitor(), MemoryMonitor(), ProcessMonitor() { // Call base class constructors
    this->updateInterval = updateInterval; //
    this->fullLog = fullLog; //
}

// Destructor
SysMon::~SysMon() {} //

int SysMon::run(int limit) { //
    // The main loop for monitoring
    int i = 0;
    while (i < limit || limit == 0) // limit=0 means infinite loop
    {
        usleep(updateInterval); // Pause for the update interval
        if (!fullLog) { //
            system("clear"); // Clear screen if not in full log mode
        }
        
        // Update all monitors
        update(); //

        // Display current system info
        displaySystemInfo();

        if (limit != 0) { // Increment only if limit is set
            i++; //
        }
    }
    return 0; //
}

bool SysMon::update() { //
    CpuMonitor::update(); //
    ProcessMonitor::update(); //
    MemoryMonitor::update(); //
    return true; //
}

// Function to get the current time for logging.
string SysMon::getTime() { //
    time_t timestamp; //
    time(&timestamp); //
    char* timeStr = ctime(&timestamp); //

    // Remove newline character
    if (timeStr[strlen(timeStr) - 1] == '\n') //
        timeStr[strlen(timeStr) - 1] = '\0'; //
    return string(timeStr); //
}

// Fetching required files '/proc/stat/' and '/proc/meminfo', etc.
string SysMon::getInfo(string _file_path) { //
    string content; //
    stringstream content_stream; //

    fstream info(_file_path, ios::in); //
    if (!info.is_open()) { // Check if file opened successfully
        // cerr << "Error: There was an error opening the file: " << _file_path << endl;
        return " "; // Return empty string to indicate error
    }
    content_stream << info.rdbuf(); // Read file content into stringstream
    content = content_stream.str(); //
    info.close(); // Close the file
    return content; //
}

void SysMon::log(ostream &out) { //
    fstream logFile("log.txt", ios::app); // Open log.txt in append mode
    if (logFile.is_open()) {
        logFile << out.rdbuf(); // Write content to log file
        logFile.close(); // Close the file
    } else {
        cerr << "Error: Could not open log.txt for writing." << endl;
    }
}

std::vector<std::string> getVector(std::istringstream &iss) { //
    std::vector<std::string> info; //
    std::string content; //
    // The original loop limit of 13 might be arbitrary for general use.
    // It's better to read until the end of the stream or specific tokens.
    // For now, keeping original logic, but be aware it might not always get all tokens needed.
    while (iss >> content) { // Read all content available
        info.push_back(content); //
    }
    return info; //
}

bool isNumber(std::string &s) { //
    for (char const &ch : s) { //
        if (std::isdigit(ch) == 0) { //
            return false; // Not a digit
        }
    }
    return true; // All characters are digits
}

// Implementation for displaying system information
void SysMon::displaySystemInfo() {
    std::cout << "\x1b[1;36m" << getTime() << "\x1b[0m\n"; // Cyan color for time

    // CPU Information
    std::cout << "\n--- CPU Info ---\n";
    std::cout << getCpuInfo(); // Displays model, cores, freq
    std::cout << "Overall CPU Usage: " << std::fixed << std::setprecision(2) << getCpuUsage() << "%\n";

    // Memory Information
    std::cout << "\n--- Memory Info ---\n";
    memUsage(0); // This also prints details to console

    // Process Information
    std::cout << "\n--- Process Info ---\n";
    std::cout << getProcessInfo(); // Displays detailed process list
}

// Export as text file
string SysMon::exportAsText() {
    std::stringstream ss;
    ss << "System Monitor Report - " << getTime() << "\n\n";

    // CPU Info
    ss << "--- CPU Information ---\n";
    ss << getCpuInfo();
    ss << "Overall CPU Usage: " << std::fixed << std::setprecision(2) << getCpuUsage() << "%\n\n";

    // Memory Info
    ss << "--- Memory Information ---\n";
    ss << "Total RAM: " << getTotalMemory() / 1024 << " MB\n";
    ss << "Used RAM: " << getUsedMemory() / 1024 << " MB\n";
    ss << "Free RAM: " << getFreeMemory() / 1024 << " MB\n";
    ss << "RAM Usage: " << getMemoryUsagePercentage() << "%\n";
    ss << "Total Swap: " << getTotalSwap() / 1024 << " MB\n";
    ss << "Used Swap: " << getUsedSwap() / 1024 << " MB\n";
    ss << "Free Swap: " << getFreeSwap() / 1024 << " MB\n";
    ss << "Swap Usage: " << getSwapUsagePercentage() << "%\n\n";

    // Process Info
    ss << "--- Process Information ---\n";
    ss << getProcessInfo(); // This already formats the process list

    SysMon::log(ss); // Log to file
    return ss.str(); // Also return as string if needed
}

// Export as CSV file
string SysMon::exportAsCSV() {
    std::stringstream ss;
    // CSV Header
    ss << "Timestamp,CPU_Usage(%),RAM_Total(MB),RAM_Used(MB),RAM_Free(MB),RAM_Usage(%),Swap_Total(MB),Swap_Used(MB),Swap_Free(MB),Swap_Usage(%)\n";
    
    // Data Row
    ss << getTime() << ","
       << std::fixed << std::setprecision(2) << getCpuUsage() << ","
       << getTotalMemory() / 1024 << ","
       << getUsedMemory() / 1024 << ","
       << getFreeMemory() / 1024 << ","
       << getMemoryUsagePercentage() << ","
       << getTotalSwap() / 1024 << ","
       << getUsedSwap() / 1024 << ","
       << getFreeSwap() / 1024 << ","
       << getSwapUsagePercentage() << "\n";

    // For processes, you might want a separate CSV or a more complex structure
    // For simplicity, let's just append process summary.
    // If detailed process list is needed in CSV, it would require a separate loop.
    // ss << "\n--- Processes ---\n";
    // ss << "PID,Name,CPU(%),Memory(MB),User,Uptime(s)\n";
    // for (const auto& proc : getActiveProcesses()) {
    //     ss << proc.pid << "," << proc.pathName << "," << proc.cpu << "," << proc.memory << ","
    //        << proc.user << "," << proc.uptime << "\n";
    // }

    fstream csvFile("log.csv", ios::app);
    if (csvFile.is_open()) {
        csvFile << ss.str();
        csvFile.close();
    } else {
        cerr << "Error: Could not open log.csv for writing." << endl;
    }
    return ss.str();
}

void SysMon::showHelp() const {
    std::cout << "Usage: SysMonApp [OPTION]\n";
    std::cout << "Monitor system resources (CPU, Memory, Processes).\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help     Display this help message and exit.\n";
    std::cout << "  --update   Perform a single update and exit.\n"; // Clarified behavior
    std::cout << "  --export   Export current system data to 'log.txt' and 'log.csv'.\n";
    std::cout << "             (Does not continuously monitor).\n";
    std::cout << "\nIf no option is provided, SysMon will run in continuous monitoring mode.\n";
}