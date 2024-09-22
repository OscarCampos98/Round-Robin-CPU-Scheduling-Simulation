#include <cstdio>
#include <string>
#include <deque>
#include <chrono>
#include <pthread.h>
#include <unistd.h>
#include <atomic>
using namespace std;

struct student
{

    int eating_time;
    int o_eating_time;         // original eating time
    int arrival_time;          // Student arrives
    int id;                    // Student ID
    long int arrival;          // Arival time IRL
    long int put_in_processor; // time when the student seates at a table
    long int waiting_time = 0; // total waiting time before seated
};

struct readyq
{
    std::deque<student> readyq;
    int quantum; // time quantom for Round-Robin
};

std::deque<student> myfile; // list of students
readyq Q;                   // the ready q
long int start_time;        // simulation time starts

atomic<bool> endfile;     // input is fully read flag
pthread_mutex_t Qlock;    // nutex for queue synchronization
pthread_cond_t has_stuff; // conditional variable to signal when queue is not empty

// color functions for printing
void cyan() { printf("\033[0;36m"); }
void white() { printf("\033[0;37m"); }
void red() { printf("\033[1;31m"); }
void yellow() { printf("\033[1;33m"); }
void green() { printf("\033[0;32m"); }

// color selection function
void color(long int diff)
{
    long int mod = diff % 5;
    switch (mod)
    {
    case 0:
        white();
        break;
    case 1:
        cyan();
        break;
    case 2:
        red();
        break;
    case 3:
        yellow();
        break;
    case 4:
        green();
        break;
    }
}

// producer thread: Reads student structure and adds it to the queue
void *producer(void *args)
{
    // initialize start time
    start_time = time(NULL);

    while (myfile.size() != 0)
    {
        student doe = myfile[0];                 // get the first student
        myfile.pop_front();                      // remove the student from the queue
        sleep(doe.arrival_time);                 // simulate the arrival time
        doe.arrival_time = (long int)time(NULL); // record the actual arrival time

        // print the arrival message
        auto diff = doe.arrival_time - start_time;
        color(diff);
        printf("ARRIVE\t%lds:\tStudent %d arrived\n", diff, doe.id);

        // lock the queue, add the student to the ready queue, signal to consumer
        pthread_mutex_lock(&Qlock);
        Q.readyq.push_back(doe);
        pthread_cond_signal(&has_stuff); // signal to the threads that there's work
        pthread_mutex_unlock(&Qlock);

        fflush(stdout);
    }

    // signal that the file has been fully read
    endfile.store(true, memory_order_relaxed);
    pthread_cond_signal(&has_stuff); // wake up all the waiting consumer threads
    pthread_exit(NULL);
    return NULL;
}

// Round-Robin customer thread: processes students at the table
void *table(void *args)
{
    while (true)
    {
        pthread_mutex_lock(&Qlock); // Lock the queue to ensure thread safety

        // If the queue is empty and there are no more students coming, exit the thread
        if (Q.readyq.size() == 0 && endfile.load(memory_order_relaxed))
        {
            pthread_mutex_unlock(&Qlock); // Unlock before exiting
            pthread_exit(NULL);
        }

        // Wait until a student is added to the queue (or the queue is non-empty)
        while (Q.readyq.size() == 0)
        {
            // Wait for the signal indicating that there are students in the queue
            pthread_cond_wait(&has_stuff, &Qlock);

            // If no more students are coming and the queue is still empty, exit
            if (Q.readyq.size() == 0 && endfile.load(memory_order_relaxed))
            {
                pthread_mutex_unlock(&Qlock);
                pthread_cond_signal(&has_stuff); // Signal any waiting threads (cleanup)
                pthread_exit(NULL);
            }
        }

        // Get the next student from the queue
        student doe = Q.readyq[0];
        Q.readyq.pop_front();
        pthread_mutex_unlock(&Qlock); // Unlock the queue after extracting the student

        // Determine how long the student will sit at the table
        int sleeping_time = 0;
        bool is_done = false;

        // If the student's remaining eating time is greater than the quantum, preempt later
        if (doe.eating_time > Q.quantum)
        {
            sleeping_time = Q.quantum; // The student will eat for one time quantum
        }
        else
        {
            sleeping_time = doe.eating_time;
            is_done = true; // The student will finish in this turn
        }

        // Record the start time of the eating process
        long int start_eat = (long int)time(NULL);
        long int diff = start_eat - start_time;
        color(diff);
        printf("SIT\t%lds:\tsit %d\n", diff, doe.id); // Log when the student sits

        sleep(sleeping_time); // Simulate the eating process

        if (is_done)
        {
            // If the student is done eating, calculate the turnaround and wait times
            long int now = (long int)time(NULL);
            long int turnaround = now - doe.arrival_time;   // Total time from arrival to leaving
            long int wait = turnaround - doe.o_eating_time; // Time spent waiting before sitting
            diff = now - start_time;                        // Time since simulation start
            color(diff);
            printf("LEAVE\t%lds:\tleave %d turnaround %ld wait %ld\n", diff, doe.id, turnaround, wait); // Log leaving
        }
        else
        {
            // If the student is not done, preempt and reinsert them back into the queue
            long int right_now = (long int)time(NULL);
            diff = right_now - start_time; // Time since simulation start
            doe.eating_time -= Q.quantum;  // Reduce the remaining eating time
            color(diff);
            printf("PREEMPT\t%ld:\tpreempt %d\n", diff, doe.id); // Log preemption

            pthread_mutex_lock(&Qlock); // Lock the queue to safely add the student back
            Q.readyq.push_back(doe);
            pthread_cond_signal(&has_stuff); // Signal other threads that there's work
            pthread_mutex_unlock(&Qlock);    // Unlock the queue
        }

        fflush(stdout); // Ensure all output is printed immediately
    }
}

int main(int argc, const char *argv[])
{
    // Check if the correct number of arguments is passed
    if (argc != 2)
    {
        printf("Hello, please give me a file!\n");
        return 0; // Exit if no file is provided
    }

    // Initialize the mutex and condition variable for thread synchronization
    pthread_mutex_init(&Qlock, NULL);
    pthread_cond_init(&has_stuff, NULL);

    // Initialize the endfile flag to false
    endfile.store(false, std::memory_order_relaxed);

    // Open the input file provided as a command-line argument
    FILE *file = fopen(argv[1], "r+");
    if (file == NULL) // Check if the file was successfully opened
    {
        printf("Error opening file!\n");
        return -1; // Exit with an error code if file couldn't be opened
    }

    // Read the quantum value (first line of the file)
    int quan;
    fscanf(file, "%d", &quan);
    Q.quantum = quan; // Set the quantum value in the ready queue

    // Variables to hold student data (arrival time and eating time)
    int eating;
    int arrive;
    int index = 0;

    // Read the student data (arrival time and eating time)
    while (fscanf(file, "%i %i", &arrive, &eating) == 2)
    {
        // Create a new student and populate their information
        student doe;
        doe.id = index;             // Assign a unique ID to each student
        doe.eating_time = eating;   // How long the student will eat
        doe.o_eating_time = eating; // Save the original eating time
        doe.arrival_time = arrive;  // The time the student arrives

        // Add the student to the input deque (myfile)
        myfile.push_back(doe);
        index += 1;
    }

    // Close the file after reading all student data
    fclose(file);

    // Record the start time of the simulation
    start_time = (long int)time(NULL);

    // Create the producer thread (to simulate student arrivals)
    pthread_t prod;
    pthread_create(&prod, NULL, producer, NULL);

    // Create four consumer threads (representing tables)
    pthread_t tables[4];
    for (int i = 0; i < 4; i++)
    {
        pthread_create(&tables[i], NULL, table, NULL);
    }

    // Wait for the producer thread to finish execution
    pthread_join(prod, NULL);

    // Wait for each table thread to finish (ensures proper cleanup)
    for (int i = 0; i < 4; i++)
    {
        pthread_join(tables[i], NULL);
    }

    // Reset the terminal color to white (optional)
    white();

    // Cleanup: Destroy the mutex and condition variable
    pthread_mutex_destroy(&Qlock);
    pthread_cond_destroy(&has_stuff);

    return 0; // Exit the program successfully
}
