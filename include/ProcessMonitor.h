#pragma once
#ifndef _PROCESSMONITOR_H
#define _PROCESSMONITOR_H

#include <vector>
#include <string>
#include <sstream>

typedef struct ap{
    float cpu;
    float memory;
    // struct time; // Removed: This was an incomplete type.
    std::string user;
    std::string pathName;
    long long uptime; // Added for process uptime if needed
    long pid; // Added for process ID
} activeProcesses;



class ProcessMonitor{
protected:
    // This should probably be a vector of activeProcesses to store multiple processes
    // activeProcesses AP; // Changed to vector
    std::vector<activeProcesses> activeProcessesList; 
    int nbrProcess; //

public:
    
    ProcessMonitor(); //

    ~ProcessMonitor(); //

    bool update(); //

    // activeProcesses getProcess(int); // This method might be better handled by returning the list
    
    std::string getProcessInfo(); //

    std::string getProcessRaw(); //

    // Added a getter for the process list
    const std::vector<activeProcesses>& getActiveProcesses() const;

};

#endif