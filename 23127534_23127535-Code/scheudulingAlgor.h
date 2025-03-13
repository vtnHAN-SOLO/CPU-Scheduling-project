#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <sstream>

using namespace std;

// Structure to represent a task (CPU or Resource)
struct Task {
    string type; 
    int duration;
    
    Task(string t, int d) : type(t), duration(d) {}
};

// Structure to represent a process
struct Process {
    int id;
    int arrivalTime;
    vector<Task> tasks;
    int currentTask;
    int remainingTime;
    int lastCpuUseTime;
    int turnaroundTime;
    int waitingTime;
    bool completed;
    
    Process(int i, int at) : id(i), arrivalTime(at), currentTask(0), 
                             remainingTime(0), lastCpuUseTime(-1),
                             turnaroundTime(0), waitingTime(0), completed(false) {}
};

//  comparing processes in SJF algorithm
struct SJFComparator {
    bool operator()(const Process* p1, const Process* p2) {
        if (p1->tasks[p1->currentTask].duration == p2->tasks[p2->currentTask].duration) {
            return p1->lastCpuUseTime > p2->lastCpuUseTime;
        }
        return p1->tasks[p1->currentTask].duration > p2->tasks[p2->currentTask].duration;
    }
};

//  comparing processes in SRTN algorithm
struct SRTNComparator {
    bool operator()(const Process* p1, const Process* p2) {
        if (p1->remainingTime == p2->remainingTime) {
            return p1->lastCpuUseTime > p2->lastCpuUseTime;
        }
        return p1->remainingTime > p2->remainingTime;
    }
};
vector<Process> parseInput(const string& filename, int& algorithm, int& timeQuantum);
vector<vector<int>> fcfsScheduling(vector<Process>& processes) ;
vector<vector<int>> rrScheduling(vector<Process>& processes, int timeQuantum);
vector<vector<int>> sjfScheduling(vector<Process>& processes);
vector<vector<int>> srtnScheduling(vector<Process>& processes);
void writeOutput(const string& filename, const vector<vector<int>>& ganttCharts, 
    const vector<Process>& processes);