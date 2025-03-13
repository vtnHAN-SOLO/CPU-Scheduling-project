#include"scheudulingAlgor.h"

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <INPUT_FILE> <OUTPUT_FILE>" << endl;
        return 1;
    }
    
    string inputFile = argv[1];
    string outputFile = argv[2];
    
    int algorithm;
    int timeQuantum = 1;
    
    vector<Process> processes = parseInput(inputFile, algorithm, timeQuantum);
    vector<vector<int>> ganttCharts;
    
    switch (algorithm) {
        case 1: 
            ganttCharts = fcfsScheduling(processes);
            break;
        case 2: 
            ganttCharts = rrScheduling(processes, timeQuantum);
            break;
        case 3: 
            ganttCharts = sjfScheduling(processes);
            break;
        case 4: 
            ganttCharts = srtnScheduling(processes);
            break;
        default:
            cout << "Invalid algorithm type!" << endl;
            return 1;
    }
    
    writeOutput(outputFile, ganttCharts, processes);
    
    return 0;
}