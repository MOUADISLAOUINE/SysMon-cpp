#include "../include/SysMon.h" //
#include <vector>
#include <string>
#include <iostream>
#include <cstring> // For strcmp

int main(int argc, char *argv[]) { //

    system("clear"); // Clear the screen on start

    int updateInterval = 10000000; // Default update interval: 10 seconds (50000000 microseconds)
    SysMon SysMonCpp(updateInterval); // Initialize SysMon object

    if (argc >= 2) { // Check for user arguments
        std::string arg = argv[1];

        if (arg == "--help") { //
            SysMonCpp.showHelp(); // Display help message
            return 0; // Exit after showing help
        } else if (arg == "--update") { //
            std::cout << "--update Called\n"; //
            SysMonCpp.update(); // Perform a single update
            SysMonCpp.displaySystemInfo(); // Display info after single update
            return 0; // Exit after single update
        } else if (arg == "--export") { //
            std::cout << "--export Called\n"; //
            SysMonCpp.update(); // Update data before exporting
            SysMonCpp.exportAsText(); // Export to text file
            SysMonCpp.exportAsCSV(); // Export to CSV file
            std::cout << "Data exported to log.txt and log.csv\n";
            return 0; // Exit after export
        } else {
            std::cout << "'" << argv[1] << "' command not recognized\n"; //
            SysMonCpp.showHelp(); // Show help for unknown command
            return -1; // Exit with error
        }
    }
    
    // Default behavior: continuous monitoring if no specific arguments
    return SysMonCpp.run(0); // Run indefinitely (limit = 0)
}