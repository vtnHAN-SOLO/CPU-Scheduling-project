#include "scheudulingAlgor.h"
vector<Process> parseInput(const string& filename, int& algorithm, int& timeQuantum) {
    ifstream infile(filename);
    string line;
    
    
    getline(infile, line);
    algorithm = stoi(line);
    
    // Read time quantum for RR
    timeQuantum = 1; // Default
    if (algorithm == 2) {
        getline(infile, line);
        timeQuantum = stoi(line);
    }
    
    // Read number of processes
    getline(infile, line);
    int numProcesses = stoi(line);
    
    vector<Process> processes;
    
    // Read process information
    for (int i = 0; i < numProcesses; i++) {
        getline(infile, line);
        istringstream iss(line);
        int at;
        iss >> at;
        
        Process p(i + 1, at);
        
        int duration;
        while (iss >> duration) {
            string taskType = "CPU";
            
            char next = iss.peek();
            if (next == '(') {
                iss.ignore();
                string resourceType;
                getline(iss, resourceType, ')');
                taskType = resourceType;
            }
            
            p.tasks.push_back(Task(taskType, duration));
        }
        
        // Set initial remaining time for the first task
        if (!p.tasks.empty() && p.tasks[0].type == "CPU") {
            p.remainingTime = p.tasks[0].duration;
        }
        
        processes.push_back(p);
    }
    
    infile.close();
    return processes;
}

// FCFS Scheduling
vector<vector<int>> fcfsScheduling(vector<Process>& processes) {
    vector<vector<int>> ganttCharts(3); // CPU, R1, R2
    vector<queue<Process*>> resourceQueues(2); // R for simplicity, R1 for index 0, R2 for index 1
    queue<Process*> readyQueue;
    
    int currentTime = 0;
    int completedProcesses = 0;
    Process* currentProcess = nullptr;
    vector<Process*> resourceProcesses(2, nullptr);
    
    // Sort processes by arrival time
    sort(processes.begin(), processes.end(), 
         [](const Process& p1, const Process& p2) { return p1.arrivalTime < p2.arrivalTime; });
    
    while (completedProcesses < processes.size()) {
        // Check for newly arrived processes
        for (auto& process : processes) {
            if (!process.completed && process.arrivalTime <= currentTime && 
                process.lastCpuUseTime == -1) {
                readyQueue.push(&process);
                process.lastCpuUseTime = 0; // Mark as entered the system
            }
        }
        
        // Check if current CPU process is done
        if (currentProcess && currentProcess->remainingTime <= 0) {
            currentProcess->tasks[currentProcess->currentTask].duration = 0;
            currentProcess->currentTask++;
            
            // If next task is Resource, add to appropriate resource queue
            if (currentProcess->currentTask < currentProcess->tasks.size()) {
                Task& nextTask = currentProcess->tasks[currentProcess->currentTask];
                
                if (nextTask.type.substr(0, 1) == "R") {
                    int resourceId = 0; // Default to R1
                    if (nextTask.type.length() > 1) {
                        resourceId = nextTask.type[1] - '1'; // Convert R1 to index 0, R2 to index 1
                    }
                    resourceQueues[resourceId].push(currentProcess);
                    currentProcess->lastCpuUseTime = currentTime;
                } else {
                    // Next task is CPU, set remaining time and add back to ready queue
                    currentProcess->remainingTime = nextTask.duration;
                    readyQueue.push(currentProcess);
                }
            } else {
                // Process completed all tasks
                currentProcess->completed = true;
                currentProcess->turnaroundTime = currentTime - currentProcess->arrivalTime;
                completedProcesses++;
            }
            
            currentProcess = nullptr;
        }
        
        // Check if resource processes are done
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i] && resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration <= 0) {
                resourceProcesses[i]->currentTask++;
                
                // If next task is CPU, add to ready queue
                if (resourceProcesses[i]->currentTask < resourceProcesses[i]->tasks.size()) {
                    Task& nextTask = resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask];
                    
                    if (nextTask.type == "CPU") {
                        resourceProcesses[i]->remainingTime = nextTask.duration;
                        readyQueue.push(resourceProcesses[i]);
                    } else {
                        // Next task is another resource
                        int nextResourceId = 0; // Default to R1
                        if (nextTask.type.length() > 1) {
                            nextResourceId = nextTask.type[1] - '1';
                        }
                        resourceQueues[nextResourceId].push(resourceProcesses[i]);
                    }
                } else {
                    // Process completed all tasks
                    resourceProcesses[i]->completed = true;
                    resourceProcesses[i]->turnaroundTime = currentTime - resourceProcesses[i]->arrivalTime;
                    completedProcesses++;
                }
                
                resourceProcesses[i] = nullptr;
            }
        }
        
        // Assign CPU to next process in ready queue
        if (!currentProcess && !readyQueue.empty()) {
            currentProcess = readyQueue.front();
            readyQueue.pop();
            currentProcess->lastCpuUseTime = currentTime;
        }
        
        // Assign resources to next processes in resource queues
        for (int i = 0; i < 2; i++) {
            if (!resourceProcesses[i] && !resourceQueues[i].empty()) {
                resourceProcesses[i] = resourceQueues[i].front();
                resourceQueues[i].pop();
            }
        }
        
        // Update Gantt charts
        if (currentProcess) {
            ganttCharts[0].push_back(currentProcess->id);
            currentProcess->remainingTime--;
        } else {
            ganttCharts[0].push_back(-1); // Idle CPU
        }
        
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i]) {
                ganttCharts[i + 1].push_back(resourceProcesses[i]->id);
                resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration--;
            } else {
                ganttCharts[i + 1].push_back(-1); // Idle resource
            }
        }
        
        // Calculate waiting time
        for (auto& process : processes) {
            if (!process.completed && process.lastCpuUseTime != -1 && 
                process.currentTask < process.tasks.size() && 
                process.tasks[process.currentTask].type == "CPU" && 
                &process != currentProcess && 
                process.arrivalTime <= currentTime) {
                
                process.waitingTime++;
            }
        }
        
        currentTime++;
    }
    
    return ganttCharts;
}

// Round Robin Scheduling
vector<vector<int>> rrScheduling(vector<Process>& processes, int timeQuantum) {
    vector<vector<int>> ganttCharts(3); // CPU, R1, R2
    vector<queue<Process*>> resourceQueues(2); // R for simplicity, R1 for index 0, R2 for index 1
    queue<Process*> readyQueue;
    
    int currentTime = 0;
    int completedProcesses = 0;
    Process* currentProcess = nullptr;
    int timeSlice = 0;
    vector<Process*> resourceProcesses(2, nullptr);
    
    // Sort processes by arrival time
    sort(processes.begin(), processes.end(), 
         [](const Process& p1, const Process& p2) { return p1.arrivalTime < p2.arrivalTime; });
    
    while (completedProcesses < processes.size()) {
        // Check for newly arrived processes
        for (auto& process : processes) {
            if (!process.completed && process.arrivalTime <= currentTime && 
                process.lastCpuUseTime == -1) {
                readyQueue.push(&process);
                process.lastCpuUseTime = 0; // Mark as entered the system
            }
        }
        
        // Check if current CPU process is done with its task or time quantum
        if (currentProcess && (currentProcess->remainingTime <= 0 || timeSlice >= timeQuantum)) {
            if (currentProcess->remainingTime <= 0) {
                // Current task is completed
                currentProcess->tasks[currentProcess->currentTask].duration = 0;
                currentProcess->currentTask++;
                
                // If next task is Resource, add to appropriate resource queue
                if (currentProcess->currentTask < currentProcess->tasks.size()) {
                    Task& nextTask = currentProcess->tasks[currentProcess->currentTask];
                    
                    if (nextTask.type.substr(0, 1) == "R") {
                        int resourceId = 0; // Default to R1
                        if (nextTask.type.length() > 1) {
                            resourceId = nextTask.type[1] - '1'; // Convert R1 to index 0, R2 to index 1
                        }
                        resourceQueues[resourceId].push(currentProcess);
                        currentProcess->lastCpuUseTime = currentTime;
                    } else {
                        // Next task is CPU, set remaining time and add back to ready queue
                        currentProcess->remainingTime = nextTask.duration;
                        readyQueue.push(currentProcess);
                    }
                } else {
                    // Process completed all tasks
                    currentProcess->completed = true;
                    currentProcess->turnaroundTime = currentTime - currentProcess->arrivalTime;
                    completedProcesses++;
                }
            } else {
                // Time quantum expired, but task not completed
                readyQueue.push(currentProcess);
            }
            
            currentProcess = nullptr;
            timeSlice = 0;
        }
        
        // Check if resource processes are done
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i] && resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration <= 0) {
                resourceProcesses[i]->currentTask++;
                
                // If next task is CPU, add to ready queue
                if (resourceProcesses[i]->currentTask < resourceProcesses[i]->tasks.size()) {
                    Task& nextTask = resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask];
                    
                    if (nextTask.type == "CPU") {
                        resourceProcesses[i]->remainingTime = nextTask.duration;
                        readyQueue.push(resourceProcesses[i]);
                    } else {
                        // Next task is another resource
                        int nextResourceId = 0; // Default to R1
                        if (nextTask.type.length() > 1) {
                            nextResourceId = nextTask.type[1] - '1';
                        }
                        resourceQueues[nextResourceId].push(resourceProcesses[i]);
                    }
                } else {
                    // Process completed all tasks
                    resourceProcesses[i]->completed = true;
                    resourceProcesses[i]->turnaroundTime = currentTime - resourceProcesses[i]->arrivalTime;
                    completedProcesses++;
                }
                
                resourceProcesses[i] = nullptr;
            }
        }
        
        // Assign CPU to next process in ready queue
        if (!currentProcess && !readyQueue.empty()) {
            currentProcess = readyQueue.front();
            readyQueue.pop();
            currentProcess->lastCpuUseTime = currentTime;
        }
        
        // Assign resources to next processes in resource queues
        for (int i = 0; i < 2; i++) {
            if (!resourceProcesses[i] && !resourceQueues[i].empty()) {
                resourceProcesses[i] = resourceQueues[i].front();
                resourceQueues[i].pop();
            }
        }
        
        // Update Gantt charts
        if (currentProcess) {
            ganttCharts[0].push_back(currentProcess->id);
            currentProcess->remainingTime--;
            timeSlice++;
        } else {
            ganttCharts[0].push_back(-1); // Idle CPU
        }
        
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i]) {
                ganttCharts[i + 1].push_back(resourceProcesses[i]->id);
                resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration--;
            } else {
                ganttCharts[i + 1].push_back(-1); // Idle resource
            }
        }
        
        // Calculate waiting time
        for (auto& process : processes) {
            if (!process.completed && process.lastCpuUseTime != -1 && 
                process.currentTask < process.tasks.size() && 
                process.tasks[process.currentTask].type == "CPU" && 
                &process != currentProcess && 
                process.arrivalTime <= currentTime) {
                
                process.waitingTime++;
            }
        }
        
        currentTime++;
    }
    
    return ganttCharts;
}

// SJF Scheduling
vector<vector<int>> sjfScheduling(vector<Process>& processes) {
    vector<vector<int>> ganttCharts(3); // CPU, R1, R2
    vector<queue<Process*>> resourceQueues(2); // R for simplicity, R1 for index 0, R2 for index 1
    priority_queue<Process*, vector<Process*>, SJFComparator> readyQueue;
    
    int currentTime = 0;
    int completedProcesses = 0;
    Process* currentProcess = nullptr;
    vector<Process*> resourceProcesses(2, nullptr);
    
    // Sort processes by arrival time
    sort(processes.begin(), processes.end(), 
         [](const Process& p1, const Process& p2) { return p1.arrivalTime < p2.arrivalTime; });
    
    while (completedProcesses < processes.size()) {
        // Check for newly arrived processes
        for (auto& process : processes) {
            if (!process.completed && process.arrivalTime <= currentTime && 
                process.lastCpuUseTime == -1) {
                readyQueue.push(&process);
                process.lastCpuUseTime = 0; // Mark as entered the system
            }
        }
        
        // Check if current CPU process is done
        if (currentProcess && currentProcess->remainingTime <= 0) {
            currentProcess->tasks[currentProcess->currentTask].duration = 0;
            currentProcess->currentTask++;
            
            // If next task is Resource, add to appropriate resource queue
            if (currentProcess->currentTask < currentProcess->tasks.size()) {
                Task& nextTask = currentProcess->tasks[currentProcess->currentTask];
                
                if (nextTask.type.substr(0, 1) == "R") {
                    int resourceId = 0; // Default to R1
                    if (nextTask.type.length() > 1) {
                        resourceId = nextTask.type[1] - '1'; // Convert R1 to index 0, R2 to index 1
                    }
                    resourceQueues[resourceId].push(currentProcess);
                    currentProcess->lastCpuUseTime = currentTime;
                } else {
                    // Next task is CPU, set remaining time and add back to ready queue
                    currentProcess->remainingTime = nextTask.duration;
                    readyQueue.push(currentProcess);
                }
            } else {
                // Process completed all tasks
                currentProcess->completed = true;
                currentProcess->turnaroundTime = currentTime - currentProcess->arrivalTime;
                completedProcesses++;
            }
            
            currentProcess = nullptr;
        }
        
        // Check if resource processes are done
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i] && resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration <= 0) {
                resourceProcesses[i]->currentTask++;
                
                // If next task is CPU, add to ready queue
                if (resourceProcesses[i]->currentTask < resourceProcesses[i]->tasks.size()) {
                    Task& nextTask = resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask];
                    
                    if (nextTask.type == "CPU") {
                        resourceProcesses[i]->remainingTime = nextTask.duration;
                        readyQueue.push(resourceProcesses[i]);
                    } else {
                        // Next task is another resource
                        int nextResourceId = 0; // Default to R1
                        if (nextTask.type.length() > 1) {
                            nextResourceId = nextTask.type[1] - '1';
                        }
                        resourceQueues[nextResourceId].push(resourceProcesses[i]);
                    }
                } else {
                    // Process completed all tasks
                    resourceProcesses[i]->completed = true;
                    resourceProcesses[i]->turnaroundTime = currentTime - resourceProcesses[i]->arrivalTime;
                    completedProcesses++;
                }
                
                resourceProcesses[i] = nullptr;
            }
        }
        
        // Assign CPU to next process in ready queue (shortest job)
        if (!currentProcess && !readyQueue.empty()) {
            currentProcess = readyQueue.top();
            readyQueue.pop();
            currentProcess->lastCpuUseTime = currentTime;
        }
        
        // Assign resources to next processes in resource queues
        for (int i = 0; i < 2; i++) {
            if (!resourceProcesses[i] && !resourceQueues[i].empty()) {
                resourceProcesses[i] = resourceQueues[i].front();
                resourceQueues[i].pop();
            }
        }
        
        // Update Gantt charts
        if (currentProcess) {
            ganttCharts[0].push_back(currentProcess->id);
            currentProcess->remainingTime--;
        } else {
            ganttCharts[0].push_back(-1); // Idle CPU
        }
        
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i]) {
                ganttCharts[i + 1].push_back(resourceProcesses[i]->id);
                resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration--;
            } else {
                ganttCharts[i + 1].push_back(-1); // Idle resource
            }
        }
        
        // Calculate waiting time
        for (auto& process : processes) {
            if (!process.completed && process.lastCpuUseTime != -1 && 
                process.currentTask < process.tasks.size() && 
                process.tasks[process.currentTask].type == "CPU" && 
                &process != currentProcess && 
                process.arrivalTime <= currentTime) {
                
                process.waitingTime++;
            }
        }
        
        currentTime++;
    }
    
    return ganttCharts;
}

// SRTN Scheduling
vector<vector<int>> srtnScheduling(vector<Process>& processes) {
    vector<vector<int>> ganttCharts(3); // CPU, R1, R2
    vector<queue<Process*>> resourceQueues(2); // R for simplicity, R1 for index 0, R2 for index 1
    priority_queue<Process*, vector<Process*>, SRTNComparator> readyQueue;
    
    int currentTime = 0;
    int completedProcesses = 0;
    Process* currentProcess = nullptr;
    vector<Process*> resourceProcesses(2, nullptr);
    
    // Sort processes by arrival time
    sort(processes.begin(), processes.end(), 
         [](const Process& p1, const Process& p2) { return p1.arrivalTime < p2.arrivalTime; });
    
    while (completedProcesses < processes.size()) {
        // Check for newly arrived processes
        for (auto& process : processes) {
            if (!process.completed && process.arrivalTime <= currentTime && 
                process.lastCpuUseTime == -1) {
                process.lastCpuUseTime = 0; // Mark as entered the system
                
                if (process.tasks[0].type == "CPU") {
                    process.remainingTime = process.tasks[0].duration;
                    readyQueue.push(&process);
                } else {
                    // Process starts with a resource task
                    int resourceId = 0; // Default to R1
                    if (process.tasks[0].type.length() > 1) {
                        resourceId = process.tasks[0].type[1] - '1';
                    }
                    resourceQueues[resourceId].push(&process);
                }
            }
        }
        
        // Preempt current process if necessary
        if (currentProcess && !readyQueue.empty()) {
            Process* topProcess = readyQueue.top();
            
            if (topProcess->remainingTime < currentProcess->remainingTime) {
                readyQueue.pop();
                readyQueue.push(currentProcess);
                currentProcess = topProcess;
                currentProcess->lastCpuUseTime = currentTime;
            }
        }
        
        // Check if current CPU process is done
        if (currentProcess && currentProcess->remainingTime <= 0) {
            currentProcess->tasks[currentProcess->currentTask].duration = 0;
            currentProcess->currentTask++;
            
            // If next task is Resource, add to appropriate resource queue
            if (currentProcess->currentTask < currentProcess->tasks.size()) {
                Task& nextTask = currentProcess->tasks[currentProcess->currentTask];
                
                if (nextTask.type.substr(0, 1) == "R") {
                    int resourceId = 0; // Default to R1
                    if (nextTask.type.length() > 1) {
                        resourceId = nextTask.type[1] - '1'; // Convert R1 to index 0, R2 to index 1
                    }
                    resourceQueues[resourceId].push(currentProcess);
                    currentProcess->lastCpuUseTime = currentTime;
                } else {
                    // Next task is CPU, set remaining time and add back to ready queue
                    currentProcess->remainingTime = nextTask.duration;
                    readyQueue.push(currentProcess);
                }
            } else {
                // Process completed all tasks
                currentProcess->completed = true;
                currentProcess->turnaroundTime = currentTime - currentProcess->arrivalTime;
                completedProcesses++;
            }
            
            currentProcess = nullptr;
        }
        
        // Check if resource processes are done
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i] && resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration <= 0) {
                resourceProcesses[i]->currentTask++;
                
                // If next task is CPU, add to ready queue
                if (resourceProcesses[i]->currentTask < resourceProcesses[i]->tasks.size()) {
                    Task& nextTask = resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask];
                    
                    if (nextTask.type == "CPU") {
                        resourceProcesses[i]->remainingTime = nextTask.duration;
                        readyQueue.push(resourceProcesses[i]);
                    } else {
                        // Next task is another resource
                        int nextResourceId = 0; // Default to R1
                        if (nextTask.type.length() > 1) {
                            nextResourceId = nextTask.type[1] - '1';
                        }
                        resourceQueues[nextResourceId].push(resourceProcesses[i]);
                    }
                } else {
                    // Process completed all tasks
                    resourceProcesses[i]->completed = true;
                    resourceProcesses[i]->turnaroundTime = currentTime - resourceProcesses[i]->arrivalTime;
                    completedProcesses++;
                }
                
                resourceProcesses[i] = nullptr;
            }
        }
        
        // Assign CPU to next process in ready queue (shortest remaining time)
        if (!currentProcess && !readyQueue.empty()) {
            currentProcess = readyQueue.top();
            readyQueue.pop();
            currentProcess->lastCpuUseTime = currentTime;
        }
        
        // Assign resources to next processes in resource queues
        for (int i = 0; i < 2; i++) {
            if (!resourceProcesses[i] && !resourceQueues[i].empty()) {
                resourceProcesses[i] = resourceQueues[i].front();
                resourceQueues[i].pop();
            }
        }
        
        // Update Gantt charts
        if (currentProcess) {
            ganttCharts[0].push_back(currentProcess->id);
            currentProcess->remainingTime--;
        } else {
            ganttCharts[0].push_back(-1); // Idle CPU
        }
        
        for (int i = 0; i < 2; i++) {
            if (resourceProcesses[i]) {
                ganttCharts[i + 1].push_back(resourceProcesses[i]->id);
                resourceProcesses[i]->tasks[resourceProcesses[i]->currentTask].duration--;
            } else {
                ganttCharts[i + 1].push_back(-1); // Idle resource
            }
        }
        
        // Calculate waiting time
        for (auto& process : processes) {
            if (!process.completed && process.lastCpuUseTime != -1 && 
                process.currentTask < process.tasks.size() && 
                process.tasks[process.currentTask].type == "CPU" && 
                &process != currentProcess && 
                process.arrivalTime <= currentTime) {
                
                process.waitingTime++;
            }
        }
        
        currentTime++;
    }
    
    return ganttCharts;
}

// Write output file
void writeOutput(const string& filename, const vector<vector<int>>& ganttCharts, 
                 const vector<Process>& processes) {
    ofstream outfile(filename);
    
    // Write Gantt charts
    for (const auto& chart : ganttCharts) {
        bool chartUsed = false;
        for (int val : chart) {
            if (val != -1) {
                chartUsed = true;
                break;
            }
        }
        if(chartUsed){    
            for (int i = 0; i < chart.size(); i++) {
                if (chart[i] == -1) {
                    outfile << "_ ";
                } else {
                    outfile << chart[i] << " ";
                }
            }
        outfile << endl;
        }
    }
    // Write turnaround times
    for (const auto& process : processes) {
        outfile << process.turnaroundTime << " ";
    }
    outfile << endl;
    
    // Write waiting times
    for (const auto& process : processes) {
        outfile << process.waitingTime << " ";
    }
    outfile << endl;
    
    outfile.close();
}
