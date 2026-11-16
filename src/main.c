#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "shared_memory.h"
#include "logger.h"

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define GET_PID() _getpid()
    #define SLEEP_MS(ms) Sleep(ms)
    unsigned __stdcall input_thread(void* arg) {
#else
    #include <unistd.h>
    #include <pthread.h>
    #define GET_PID() getpid()
    #define SLEEP_MS(ms) usleep((ms) * 1000)
    void* input_thread(void* arg) {
#endif
    shared_data_t* data = (shared_data_t*)arg;
    char input[100];
    int val;
    while (1) {
        printf("Enter new counter value (or 'q' to quit): ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        if (input[0] == 'q') break;
        if (sscanf(input, "%d", &val) == 1) {
            lock_memory(data);
            data->counter = val;
            unlock_memory(data);
            printf("Counter set to %d\n", val);
        }
    }
    return 0;
}

int main() {
    int pid = GET_PID();
    
    // 1. Init Logger
    init_logger();
    write_log(pid, 0, "PROGRAM STARTED");

    // 2. Init Shared Memory
    shared_data_t* data = create_shared_memory();
    if (!data) {
        printf("Failed to create shared memory.\n");
        return 1;
    }

    // Initialize counter and master PID (only if we are the first instance)
    if (data->counter == 0 && data->master_pid == 0) {
        data->counter = 0;
        data->master_pid = pid;
    }

    // 3. Start Input Thread
    #ifdef _WIN32
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, input_thread, data, 0, NULL);
    #else
        pthread_t thread;
        pthread_create(&thread, NULL, input_thread, data);
    #endif

    // 4. Main Loop (Timers)
    clock_t last_increment = clock();
    time_t last_log = time(NULL);
    
    while (1) {
        clock_t now = clock();
        
        // 300ms Counter Increment
        if ((now - last_increment) * 1000 / CLOCKS_PER_SEC >= 300) {
            lock_memory(data);
            data->counter++;
            unlock_memory(data);
            last_increment = now;
        }

        // 1-second Log Write
        time_t current_time = time(NULL);
        if (current_time - last_log >= 1) {
            lock_memory(data);
            int current_counter = data->counter;
            unlock_memory(data);
            
            write_log(pid, current_counter, "TICK");
            last_log = current_time;
        }

        // Small sleep to prevent CPU 100% usage
        SLEEP_MS(10); 
    }

    // Cleanup (unreachable in this infinite loop, but good practice)
    detach_shared_memory(data);
    close_logger();
    return 0;
}