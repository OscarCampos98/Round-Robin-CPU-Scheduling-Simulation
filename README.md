# Round-Robin CPU Scheduling Simulation

# Overview:
This program simulates a round-robin CPU scheduling algorithm for students arriving at a dining center with a limited number
of tables. Students arrive at different times and each has a specified amount of time required to eat. The scheduling algorithm
allocates a fixed time quantum for each student to sit at a table. If a student's eating time exceeds the quantum, they are preempted
and must wait for their next turn.

# Problem Description
The program is designed to:
    - Simulate a dining center with multiple tables.
    - Manage student arrivals, seating, and eating using a round-robin scheduling algorithm.
    - Each student arrives at a specific time and requires a certain amount of time to eat.
    - If the student's eating time exceeds the quantum, they are preempted and put back into the queue.

The goal is to minimize the waiting time and turnaround time for students while ensuring fair allocation of dining resources (tables).

# Files Included
roundrobin.cpp: The main source code implementing the round-robin scheduling algorithm using pthreads.
.txt test files: Example input file containing student arrival and eating times and burst (this can be customized).

# Compilation
To compile the program, ensure that you have g++ installed and that it supports the pthread library. Run the following command: 
i am using the WSL:Ubuntu-20.04 linux terminal to run 

g++ roundrobin.cpp -o round -pthread
./round <input_file.txt>

# Input File Format
The first line of the input file specifies the time quantum for the round-robin scheduling.
Each subsequent line contains two integers:
The arrival time (in seconds) – the time at which the student arrives at the dining center.
The eating time (in seconds) – the total amount of time the student needs to eat.

Example Input:

    5         # Quantum of 5 seconds
    0 5       # Student arrives at time 0, needs 5 seconds to eat
    2 7       # Student arrives at time 2, needs 7 seconds to eat

# Output Format
The output of the program logs four key events:

ARRIVE <timestamp>: Student <id> arrived: Indicates when a student arrives at the dining center.
SIT <timestamp>: Student <id> sits: Indicates when a student sits at a table to begin eating.
PREEMPT <timestamp>: Student <id> preempted: Indicates when a student is preempted (if their eating time exceeds the quantum).
LEAVE <timestamp>: Student <id> leaves. Turnaround <time> Wait <time>: Indicates when a student finishes eating and leaves the dining center. The log includes the total time they spent in the dining center (turnaround time) and the total time they spent waiting for a table.

Example Output:

    ARRIVE 0s:      Student 0 arrived
    SIT 0s:         Student 0 sits
    ARRIVE 2s:      Student 1 arrived
    SIT 2s:         Student 1 sits
    ARRIVE 4s:      Student 3 arrived
    SIT 4s:         Student 3 sits
    LEAVE 5s:       Student 0 leaves. Turnaround 5 Wait 0

# Important Concepts
Turnaround Time: The total time from a student's arrival to their departure (includes both eating and waiting time).
Waiting Time: The time a student spends waiting for a table after arriving.

# Error Handling
If an invalid file is provided or the file cannot be opened, the program will return an error message.
If the input file does not follow the required format, the program will terminate with an appropriate error.

# How the Code Works
Producer-Consumer Model: The program follows a producer-consumer model. The producer thread is responsible for reading
student data (arrival and eating times) from the input file and adding each student to the ready queue.

Four consumer threads represent the dining tables. Each consumer thread pulls a student from the queue, allows them to sit and eat 
for a specific time quantum, and either finishes their meal or preempts them if their eating time exceeds the quantum.

Round-Robin Scheduling: The round-robin scheduling algorithm is implemented with the pthreads library to ensure fairness in allocating 
table time to students. Each student gets up to the quantum amount of time to eat. If they don’t finish in that time,
they are preempted and returned to the queue. This ensures fairness by preventing any student from monopolizing a table for too long.

Synchronization: The program uses mutex locks and condition variables to manage synchronization between the producer and consumer threads.
    - The queue is accessed in a safe and orderly manner, preventing race conditions.
    - Threads are efficiently coordinated using condition variables to wait for work and resume when students are available.
    - Preemptions are handled fairly, ensuring no student gets more time than allotted by the quantum.


# Future Improvements
Allow for dynamic table allocation based on input.
Implement a graphical representation of the scheduling process.
Add more detailed logging for debugging purposes.
