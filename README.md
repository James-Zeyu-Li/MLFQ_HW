# CS5600 MLFQ
 

How to run the simulator: 
    ```cpp
    g++ -std=c++20 main.cpp -o scheduler       
    ```
`./scheduler`

### Default Value without user flag
- **0 is the highest Level priority, the number of queue is the lowest priority**
- Default 2 process, 3 queues
- Time slice for queue 1,2,3
- Allowance for  queue 4,6,8
- booster - Implemented but not working as expected
- IO frequency , I tried really hard but unfortunately, I was unable to add this to my code. 



### Command options:
`-s SEED: Set random seed`

`-n NUMQUEUES: Number of queues in MLFQ`

`-Q QUANTUMLIST: Comma-separated list of quantum per queue level (high to low priority)`

`-A ALLOTMENTLIST: Comma-separated list of allotment per queue level (high to low priority)`

`-b BOOSTTIME: Time interval for boosting priorities`

`-j JOBLIST: Semicolon-separated list of jobs, each in format id,startTime,totalTime`
- When running -j for job list please do it like the following command
    ` -j "1,0,10;2,0,5;3,4,10"`

`-h: Show this help message and exit`
