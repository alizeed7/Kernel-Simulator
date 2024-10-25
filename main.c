/* Assignment 1 - Part 2
 * 
 * @author			Ryan Capstick - 101239778
 * @author			Alizee Drolet - 101193138
 * 			
 * @version			v1.00
 * @release			October 13, 2023
 *
 * A small simulator of an OS kernel, which could be used for
 * performance analysis of different scheduling algorithms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

typedef struct PCB {
    int PID;               // a unique identifier for the process
    int arrivalTime;       // in milliseconds
    int CPUTime;           // total time the process needs to complete in milliseconds (excluding I/O)
    int freq;              // the processes make a call to an event and wait with this frequency
    int duration;          // duration the process must wait before the event completion
    int remainingCPUTime;  // remaining time to complete CPU processing
    int waitStartTime;     //time when the process enters the waiting queue
    struct PCB *next;
} PCB;

typedef struct queue {
    PCB *front;
    PCB *rear;
    int size;
} queue_t;

bool isEmpty(queue_t *queue) {
    return queue->size == 0;
}

// Allocate and initialize functions for the queue
queue_t *new_queue(void) {
    queue_t *queue = (queue_t *)malloc(sizeof(queue_t));
    assert(queue != NULL);

    // Initialize elements of the queue
    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;

    return queue;
}

void enqueue(queue_t *queue, PCB *pcb) {
    // If the queue is empty, add the PCB node at the front; else, add it to the end
    if (queue->front == NULL) {
        queue->front = pcb;
    } else {
        queue->rear->next = pcb;
    }

    queue->rear = pcb;
    queue->size += 1;
}

PCB *dequeue(queue_t *queue) {
    if (queue->size == 0) {
        // Cannot dequeue from an empty queue
        return NULL;
    } else {
        PCB *temp = queue->front;
        queue->front = queue->front->next;
        queue->size -= 1;
        return temp;
    }
}

// for debugging to view transitions
void printQueues(queue_t *ready, queue_t *waiting, queue_t *terminated) {
    printf("Ready Queue: ");
    PCB *current = ready->front;
    while (current != NULL) {
        printf("PID %d, ", current->PID);
        current = current->next;
    }
    printf("\n");

    printf("Waiting Queue: ");
    current = waiting->front;
    while (current != NULL) {
        printf("PID %d, ", current->PID);
        current = current->next;
    }
    printf("\n");

    printf("Terminated Queue: ");
    current = terminated->front;
    while (current != NULL) {
        printf("PID %d, ", current->PID);
        current = current->next;
    }
    printf("\n");
}

// get input data from the input file and put it in an array of PCB structs
int getData(char fileName[], PCB **processes) {
    FILE *csvFile;
    int num_processes = 0;

    // Open the CSV file and check if it exists
    csvFile = fopen(fileName, "r");
    if (csvFile == NULL) {
        perror("File does not exist");
        return -1;
    }

    // Skip the header line
    char header[256];
    fgets(header, sizeof(header), csvFile);

    // Count the number of entries (number of processes)
    char ch;
    while ((ch = fgetc(csvFile)) != EOF) {
        if (ch == '\n') {
            num_processes++;
        }
    }

    printf("%d\n", num_processes);

    *processes = (PCB *)malloc(num_processes * sizeof(PCB));
    if (*processes == NULL) {
        perror("Cannot allocate memory");
        fclose(csvFile);
        return -1;
    }

    // Rewind the file to the beginning
    rewind(csvFile);

    // Skip the header line again
    fgets(header, sizeof(header), csvFile);

    // Populate the PCB struct variables according to the file
    // for (int i = 0; i < num_processes; i++) {
    //     fscanf(csvFile, "%d, %d, %d, %d, %d", &(*processes)[i].PID, &(*processes)[i].arrivalTime, &(*processes)[i].CPUTime, &(*processes)[i].freq, &(*processes)[i].duration);
    //     processes[i]->remainingCPUTime = processes[i]->CPUTime;
    //     printf("process: %d, PID: %d\n", i, processes[i]->PID);
    // }
    for (int i = 0; i < num_processes; i++) {
        if (fscanf(csvFile, "%d, %d, %d, %d, %d", &(*processes)[i].PID, &(*processes)[i].arrivalTime, &(*processes)[i].CPUTime, &(*processes)[i].freq, &(*processes)[i].duration) != 5) {
            perror("Error reading data from the file");
            free(*processes);
            fclose(csvFile);
            return -1;
        }
        (*processes)[i].remainingCPUTime = (*processes)[i].CPUTime;
        printf("process: %d, PID: %d\n", i, (*processes)[i].PID);
    }

    fclose(csvFile);
    return num_processes;
}

void outputTransition(FILE *outputFile, int clk, int PID, const char *oldState, const char *newState) {
    fprintf(outputFile, "%d %d %s %s\n", clk, PID, oldState, newState);
    // printf("%d %d %s %s\n", clk, PID, oldState, newState);
}

void kernelSim(PCB *processes, int num_processes, const char *outputFileName) {
    int clk = 0;
    queue_t *ready = new_queue();
    queue_t *waiting = new_queue();
    queue_t *terminated = new_queue();

    // printf("Ready size: %d", ready->size);

    // open a new file to write the output
    FILE *outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        perror("Cannot create the output file");
        return;
    }

    fprintf(outputFile, "Time PID OldState NewState\n");

    while (terminated->size < num_processes) {
        // Check for processes arriving at the current time and move them to the ready queue
        for (int i = 0; i < num_processes; i++) {
            if (processes[i].arrivalTime == clk) { 
                outputTransition(outputFile, clk, processes[i].PID, "New", "Ready");
                enqueue(ready, &processes[i]);
            }
        }

        // Execute the processes in the ready queue
        PCB *currentProcess = dequeue(ready);
        if (currentProcess != NULL) {
            outputTransition(outputFile, clk, currentProcess->PID, "Ready", "Running");
            if (currentProcess->remainingCPUTime <= currentProcess->freq) {
                // Process finishes its CPU burst
                outputTransition(outputFile, clk /*+ currentProcess->remainingCPUTime*/, currentProcess->PID, "Running", "Terminated");
                currentProcess->remainingCPUTime = 0;
                enqueue(terminated, currentProcess);
            } else {
                // Process needs to perform I/O
                outputTransition(outputFile, clk /*+ currentProcess->freq*/, currentProcess->PID, "Running", "Waiting");
                currentProcess->waitStartTime = clk;
                currentProcess->remainingCPUTime -= currentProcess->freq;
                // currentProcess->arrivalTime = clk + currentProcess->freq + currentProcess->duration;
                enqueue(waiting, currentProcess);
            }
        }

        // Increment the simulation time (clk) and handle I/O completion
        clk++;

        // Check for I/O completions
        PCB *currentIOProcess = waiting->front;
        while (currentIOProcess != NULL) {
            if ((clk - currentIOProcess->waitStartTime) == currentIOProcess->duration){
                outputTransition(outputFile, clk, currentIOProcess->PID, "Waiting", "Ready");
                enqueue(ready, currentIOProcess);
                currentIOProcess = dequeue(waiting);

            } else {
                break;
            }
            currentIOProcess = waiting->front;
        }

        // Print the state of queues for debugging and monitoring
        // printQueues(ready, waiting, terminated);
    }

    fclose(outputFile);

    // Free the memory for the queues
    free(ready);
    free(waiting);
    free(terminated);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input_file.csv>\n", argv[0]);
        return 1;
    }

    char *inputFileName = argv[1];
    PCB *processes = NULL;

    int num_processes = getData(inputFileName, &processes);
    if (num_processes > 0) {
        // Generate an output file name based on the input file name
        char outputFileName[200];
        snprintf(outputFileName, sizeof(outputFileName), "output_%s.txt", inputFileName);

        kernelSim(processes, num_processes, outputFileName);
        free(processes);
    }
    return 0;
}
