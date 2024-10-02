#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <cstdlib>
#include <string>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <iomanip>

using namespace std;

struct Job {
    int id;
    int startTime;
    int totalTime;
    int remainingTime;
    int currPri;
    int allotLeft;
    int firstRun;
    int responseTime;
    int endTime;
    bool started;
};

// Global Variables
int hiQueue;
vector<queue<int>> queues;
map<int, Job> jobs;
vector<int> quantumList;    // Quantum per priority level (Queue 0 to Queue N-1)
vector<int> allotmentList;  // Allotment per priority level (Queue 0 to Queue N-1)
int boostTime;
int numQueues;
int currentTime = 0;
int nextBoost = 0;

// Command-line option structure, use to transfer command-line arguments
struct Options {
    // Simulation parameters
    int seed = 0;
    int numQueues = 3;
    vector<int> quantumList = {1, 2, 3};     // Default quantum list (Queue 0 to Queue 2)
    vector<int> allotmentList = {4, 6, 8};   // Default allotment list (Queue 0 to Queue 2)
    int boostTime = 20;
    // Jobs
    vector<Job> jobList = {
        Job{1, 0, 10, 10, 0, 4, -1, 0, 0, false}, // Default job list. Will be overwritten by -j option
        Job{2, 5, 10, 10, 0, 4, -1, 0, 0, false}
    };
};

// Error handling function
void Abort(const string& str) {
    cerr << "Error: " << str << endl;
    exit(1);
}

// Split a string into a vector of strings, helper function
void splitString(const string& str, char delimiter, vector<string>& out) {
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        out.push_back(token);
    }
}

// Split a string into a vector of integers, helper function
void splitStringToInt(const string& str, char delimiter, vector<int>& out) {
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        try {
            out.push_back(stoi(token));
        } catch (invalid_argument &e) {
            cerr << "Invalid number in list: " << token << endl;
            exit(1);
        }
    }
}

// Function to parse command-line arguments
void parseArguments(int argc, char* argv[], Options& options) {
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-s") {
            if (i + 1 < argc) {
                options.seed = stoi(argv[++i]);
            } else {
                Abort("Missing value for -s");
            }
        } else if (arg == "-n") {
            if (i + 1 < argc) {
                options.numQueues = stoi(argv[++i]);
            } else {
                Abort("Missing value for -n");
            }
        } else if (arg == "-Q") {
            if (i + 1 < argc) {
                options.quantumList.clear();
                splitStringToInt(argv[++i], ',', options.quantumList);
            } else {
                Abort("Missing value for -Q");
            }
        } else if (arg == "-A") {
            if (i + 1 < argc) {
                options.allotmentList.clear();
                splitStringToInt(argv[++i], ',', options.allotmentList);
            } else {
                Abort("Missing value for -A");
            }
        } else if (arg == "-b") {
            if (i + 1 < argc) {
                options.boostTime = stoi(argv[++i]);
            } else {
                Abort("Missing value for -b");
            }
        } else if (arg == "-j") {
            if (i + 1 < argc) {
                // Clear job list, then parse the new list, format: id,startTime,totalTime
                options.jobList.clear(); // Clear default jobs
                string jobStr = argv[++i]; 
                vector<string> jobsStr;
                splitString(jobStr, ';', jobsStr);
                for (const auto& jobEntry : jobsStr) {
                    vector<string> jobFields;
                    splitString(jobEntry, ',', jobFields);
                    if (jobFields.size() != 3) {
                        Abort("Invalid job format. Expected id,startTime,totalTime");
                    }
                    Job job;
                    job.id = stoi(jobFields[0]);
                    job.startTime = stoi(jobFields[1]);
                    job.totalTime = stoi(jobFields[2]);
                    job.remainingTime = job.totalTime;
                    job.currPri = 0; // To be set in setupJobs
                    job.allotLeft = 0; // To be set in setupJobs
                    job.firstRun = -1;
                    job.responseTime = 0;
                    job.endTime = 0;
                    job.started = false;
                    options.jobList.push_back(job);
                }
            } else {
                Abort("Missing value for -j");
            }
        } else if (arg == "-h") {
            cout << "Command options:\n"
                 << "-s SEED: Set random seed\n"
                 << "-n NUMQUEUES: Number of queues in MLFQ\n"
                 << "-Q QUANTUMLIST: Comma-separated list of quantum per queue level (high to low priority)\n"
                 << "-A ALLOTMENTLIST: Comma-separated list of allotment per queue level (high to low priority)\n"
                 << "-b BOOSTTIME: Time interval for boosting priorities\n"
                 << "-j JOBLIST: Semicolon-separated list of jobs, each in format id,startTime,totalTime\n"
                 << "-h: Show this help message and exit\n";
            exit(0);
        } else {
            Abort("Unknown argument: " + arg);
        }
    }
}

// Initialize quantum and allotment lists with padding if necessary
void initializeQuantumAndAllotment(const Options& options) {
    // If quantumList empty, use default {1, 2, 3}
    if (options.quantumList.empty()) {
        quantumList = {1, 2, 3};
    } else {
        quantumList = options.quantumList;
    }

    // If allotmentList empty, use default {4, 6, 8}
    if (options.allotmentList.empty()) {
        allotmentList = {4, 6, 8};
    } else {
        allotmentList = options.allotmentList;
    }

    // Define default quantum and allotment values
    const int defaultQuantum = 5;     // Adjusted to the highest quantum
    const int defaultAllotment = 20;  // Adjusted to the highest allotment

    // Match quantumList and allotmentList to numQueues
    if (quantumList.size() < options.numQueues) {
        int lastQuantum = quantumList.empty() ? defaultQuantum : quantumList.back();
        while (quantumList.size() < options.numQueues) {
            quantumList.push_back(lastQuantum);
        }
    }

    // Match allotmentList to numQueues
    if (allotmentList.size() < options.numQueues) {
        int lastAllotment = allotmentList.empty() ? defaultAllotment : allotmentList.back();
        while (allotmentList.size() < options.numQueues) {
            allotmentList.push_back(lastAllotment);
        }
    }

    // Resize quantumList and allotmentList to numQueues
    if (quantumList.size() > options.numQueues) {
        quantumList.resize(options.numQueues);
    }
    if (allotmentList.size() > options.numQueues) {
        allotmentList.resize(options.numQueues);
    }

    //  Display priority allotments
    cout << "Priority Allotments:\n";
    for(int i=0; i<numQueues; i++) {
        cout << "Priority " << i << " allotment: " << allotmentList[i]
             << " | Quantum: " << quantumList[i] << endl;
    }
    cout << endl;
}

// Setup jobs based on options
void setupJobs(const Options& options) {
    for (const auto& job : options.jobList) {
        Job newJob = job;
        newJob.currPri = 0; // Set to highest priority (Queue 0)
        newJob.allotLeft = allotmentList[0];
        jobs[newJob.id] = newJob;
    }
}

// Function to find the next job to run
int FindQueue() {
    for (int q = 0; q < hiQueue + 1; q++) { // Iterate from 0 (highest) to hiQueue
        if (!queues[q].empty()) {
            return q;
        }
    }
    return -1;
}

// Boost all jobs to the highest priority queue
void boostAllQueues() {
    // Move all jobs back to the highest priority queue
    cout << "Boosting all jobs to highest priority at time " << currentTime << endl;
    for (int q = 1; q <= hiQueue; q++) {
        while (!queues[q].empty()) {
            int jobId = queues[q].front();
            queues[q].pop();
            jobs[jobId].currPri = 0; // Highest priority
            jobs[jobId].allotLeft = allotmentList[0];
            queues[0].push(jobId);
        }
    }
    // Reset next boost time
    nextBoost += boostTime;
}

// Run the MLFQ simulation
void runSimulation(const Options& options) {
    // Initialize variables
    hiQueue = options.numQueues - 1;
    queues.resize(options.numQueues);

    // Initialize boost timing
    boostTime = options.boostTime;
    nextBoost = boostTime;

    // Simulation loop
    while (true) {
        // Check for boost
        if (currentTime == nextBoost) {
            boostAllQueues();
        }

        // Add jobs that arrive at current time to the highest priority queue
        for (auto& [id, job] : jobs) {
            if (job.startTime == currentTime) {
                queues[0].push(id); // Highest priority queue
                job.currPri = 0;
                job.allotLeft = allotmentList[0];
                cout << "Initialized Job " << id << " with startTime " << job.startTime 
                     << " and totalTime " << job.totalTime << endl;
                cout << "Job " << id << " arrived at time " << currentTime 
                     << " and added to queue " << job.currPri << endl;
            }
        }

        // Find the next job to run
        int q = FindQueue();
        if (q == -1) {
            // No jobs in queues; check if all jobs are finished
            bool allDone = true;
            for (const auto& [id, job] : jobs) {
                if (job.remainingTime > 0) {
                    allDone = false;
                    break;
                }
            }
            if (allDone) {
                break; // Simulation finished
            }
            //  No jobs to run; move to next time unit
            currentTime++;
            continue;
        }

        // Get the next job from queue q
        int jobId = queues[q].front();
        queues[q].pop();
        Job& job = jobs[jobId];

        // Start or resume the job, and print the status
        if (!job.started) {
            job.firstRun = currentTime;
            job.responseTime = currentTime - job.startTime;
            job.started = true;
            cout << "Job " << jobId << " started at time " << currentTime 
                 << " with priority " << q 
                 << " (Quantum: " << quantumList[q] 
                 << ", Allotment: " << job.allotLeft << ")" << endl;
        } else {
            cout << "Job " << jobId << " resumed at time " << currentTime 
                 << " with priority " << q 
                 << " (Quantum: " << quantumList[q] 
                 << ", Allotment left: " << job.allotLeft << ")" << endl;
        }

        // Determine the time slice
        int quantum = quantumList[q];
        int timeSlice = min({quantum, job.remainingTime, job.allotLeft});
        int timeUsed = 0;

        // Execute the job for the time slice
        while (timeUsed < timeSlice) {
            currentTime++;
            job.remainingTime--;
            timeUsed++;

            // Check for boost during execution
            if (currentTime == nextBoost) {
                boostAllQueues();
            }

            // Add jobs that arrive at current time to the highest priority queue
            for (auto& [id, newJob] : jobs) {
                if (newJob.startTime == currentTime) {
                    queues[0].push(id); // Highest priority queue
                    newJob.currPri = 0;
                    newJob.allotLeft = allotmentList[0];
                    cout << "Initialized Job " << id << " with startTime " << newJob.startTime 
                         << " and totalTime " << newJob.totalTime << endl;
                    cout << "Job " << id << " arrived at time " << currentTime 
                         << " and added to queue " << newJob.currPri << endl;
                }
            }

            // Check if the job is finished
            if (job.remainingTime == 0) {
                job.endTime = currentTime;
                cout << "Job " << jobId << " finished at time " << currentTime << endl;
                break;
            }
        }

        //  Check if the job used up its time slice
        if (job.remainingTime > 0) {
            // Decrement allotment
            job.allotLeft -= timeUsed;
            cout << "Job " << jobId << " used " << timeUsed 
                 << " units at priority " << q 
                 << " (Quantum used: " << timeUsed 
                 << ", Allotment left: " << job.allotLeft << ")" << endl;

            if (job.allotLeft <= 0) {
                // Allotment exhausted; demote the job
                if (job.currPri < hiQueue) {
                    job.currPri++;
                    job.allotLeft = allotmentList[job.currPri];
                    cout << "Job " << jobId << " demoted to queue " << job.currPri 
                         << " at time " << currentTime 
                         << " (Allotment reset to " << job.allotLeft << ")" << endl;
                } else {
                    // Job remains at the lowest priority
                    job.allotLeft = allotmentList[job.currPri];
                    cout << "Job " << jobId << " remains at queue " << job.currPri 
                         << " with allotment reset to " << job.allotLeft << " at time " 
                         << currentTime << endl;
                }
            }

            // Re-add the job to the appropriate queue without printing
            queues[job.currPri].push(jobId);
        }
    }
}

// Display final statistics
void displayFinalStatistics() {
    cout << "\nSimulation completed at time " << currentTime << "\n\n";
    cout << left << setw(10) << "JobID" 
         << setw(15) << "StartTime" 
         << setw(15) << "ResponseTime" 
         << setw(20) << "TurnaroundTime" << endl;
    double totalResponse = 0;
    double totalTurnaround = 0;
    for (const auto& [id, job] : jobs) {
        int turnaround = job.endTime - job.startTime;
        totalResponse += job.responseTime;
        totalTurnaround += turnaround;
        cout << left << setw(10) << id 
             << setw(15) << job.startTime 
             << setw(15) << job.responseTime 
             << setw(20) << turnaround << endl;
    }
    double avgResponse = jobs.empty() ? 0 : totalResponse / jobs.size();
    double avgTurnaround = jobs.empty() ? 0 : totalTurnaround / jobs.size();
    cout << "\nFinal statistics:\n";
    cout << "Avg\t" << setw(10) << "-" 
         << setw(15) << fixed << setprecision(2) << avgResponse 
         << setw(20) << fixed << setprecision(2) << avgTurnaround << endl;
}

int main(int argc, char* argv[]) {
    Options options;

    // Parse command-line arguments
    parseArguments(argc, argv, options);

    // Initialize quantum and allotment lists
    numQueues = options.numQueues;
    initializeQuantumAndAllotment(options);

    // Setup jobs
    setupJobs(options);

    // Set boostTime from user-specified option
    boostTime = options.boostTime;

    // If a random seed is set (not used )
    if (options.seed != 0) {
        srand(options.seed);
    }

    // Run the MLFQ simulation
    runSimulation(options);

    // Display final statistics
    displayFinalStatistics();

    return 0;
}
