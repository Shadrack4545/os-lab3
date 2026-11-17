#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "shared_memory.h"
#include "logger.h"

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #define GET_PID() GetCurrentProcessId()
    #define SLEEP_MS(ms) Sleep(ms)
    unsigned __stdcall input_thread(void* arg) {
#else
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/wait.h>
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

/* =========================================================
   COPY 1 LOGIC: Writes start, adds 10, writes exit, terminates.
   ========================================================= */
void run_copy1(shared_data_t* data) {
    process_id_t pid = GET_PID();
    init_logger();
    
    lock_memory(data);
    write_log(pid, data->counter, "COPY 1 STARTED");
    data->counter += 10;
    write_log(pid, data->counter, "COPY 1 ADDED 10");
    unlock_memory(data);
    
    write_log(pid, data->counter, "COPY 1 EXITING");
    close_logger();
    exit(0);
}

/* =========================================================
   COPY 2 LOGIC: Writes start, *2, wait 2s, /2, exit.
   ========================================================= */
void run_copy2(shared_data_t* data) {
    process_id_t pid = GET_PID();
    init_logger();
    
    lock_memory(data);
    write_log(pid, data->counter, "COPY 2 STARTED");
    data->counter *= 2;
    write_log(pid, data->counter, "COPY 2 MULTIPLIED BY 2");
    unlock_memory(data);
    
    // Wait 2 seconds (outside the lock so we don't block others)
    SLEEP_MS(2000);
    
    lock_memory(data);
    data->counter /= 2;
    write_log(pid, data->counter, "COPY 2 DIVIDED BY 2");
    unlock_memory(data);
    
    write_log(pid, data->counter, "COPY 2 EXITING");
    close_logger();
    exit(0);
}

int main(int argc, char* argv[]) {
    // 0. Check if we are running as a copy
    if (argc > 1 && strcmp(argv[1], "--copy1") == 0) {
        shared_data_t* data = attach_shared_memory();
        if (!data) return 1;
        run_copy1(data);
    }
    if (argc > 1 && strcmp(argv[1], "--copy2") == 0) {
        shared_data_t* data = attach_shared_memory();
        if (!data) return 1;
        run_copy2(data);
    }

    // --- MASTER / WORKER PROGRAM LOGIC ---
    process_id_t pid = GET_PID();
    init_logger();
    write_log(pid, 0, "PROGRAM STARTED");

    shared_data_t* data = create_shared_memory();
    if (!data) return 1;

    if (data->master_pid == 0) {
        data->counter = 0;
        data->master_pid = pid;
        write_log(pid, 0, "I AM THE MASTER");
    } else {
        write_log(pid, 0, "I AM A WORKER");
    }

    // Start Input Thread
    #ifdef _WIN32
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, input_thread, data, 0, NULL);
    #else
        pthread_t thread;
        pthread_create(&thread, NULL, input_thread, data);
    #endif

    clock_t last_increment = clock();
    time_t last_log = time(NULL);
    time_t last_spawn = time(NULL);
    
    // Variables to track child processes
    #ifdef _WIN32
        HANDLE copy1_process = NULL;
        HANDLE copy2_process = NULL;
    #else
        pid_t copy1_process = 0;
        pid_t copy2_process = 0;
    #endif

    while (1) {
        clock_t now = clock();
        time_t current_time = time(NULL);
        
        // 300ms Counter Increment
        if ((now - last_increment) * 1000 / CLOCKS_PER_SEC >= 300) {
            lock_memory(data);
            data->counter++;
            unlock_memory(data);
            last_increment = now;
        }

        // 1-second Log Write (ONLY the Master should write the TICK log)
        if (current_time - last_log >= 1 && pid == data->master_pid) {
            lock_memory(data);
            int current_counter = data->counter;
            unlock_memory(data);
            
            write_log(pid, current_counter, "TICK");
            last_log = current_time;
        }

        // 3-second Spawn Logic (ONLY the Master should spawn copies)
        if (current_time - last_spawn >= 3 && pid == data->master_pid) {
            // Check if Copy 1 has finished (Req 5c)
            if (copy1_process != NULL) {
                #ifdef _WIN32
                    DWORD waitStatus = WaitForSingleObject(copy1_process, 0);
                    if (waitStatus == WAIT_TIMEOUT) {
                        lock_memory(data);
                        write_log(pid, data->counter, "SKIPPED COPY 1 (still running)");
                        unlock_memory(data);
                    } else {
                        CloseHandle(copy1_process);
                        copy1_process = NULL;
                    }
                #else
                    if (waitpid(copy1_process, NULL, WNOHANG) == 0) {
                        lock_memory(data);
                        write_log(pid, data->counter, "SKIPPED COPY 1 (still running)");
                        unlock_memory(data);
                    } else {
                        copy1_process = 0;
                    }
                #endif
            }
            
            // Check if Copy 2 has finished (Req 5c)
            if (copy2_process != NULL) {
                #ifdef _WIN32
                    DWORD waitStatus = WaitForSingleObject(copy2_process, 0);
                    if (waitStatus == WAIT_TIMEOUT) {
                        lock_memory(data);
                        write_log(pid, data->counter, "SKIPPED COPY 2 (still running)");
                        unlock_memory(data);
                    } else {
                        CloseHandle(copy2_process);
                        copy2_process = NULL;
                    }
                #else
                    if (waitpid(copy2_process, NULL, WNOHANG) == 0) {
                        lock_memory(data);
                        write_log(pid, data->counter, "SKIPPED COPY 2 (still running)");
                        unlock_memory(data);
                    } else {
                        copy2_process = 0;
                    }
                #endif
            }

            // If processes are NULL/0, it means they are free to launch
            if (copy1_process == NULL) { 
                #ifdef _WIN32
                    STARTUPINFOA si = {0}; PROCESS_INFORMATION pi = {0};
                    si.cb = sizeof(si);
                    char cmd[] = "lab3.exe --copy1";
                    CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                    copy1_process = pi.hProcess;
                    CloseHandle(pi.hThread);
                #else
                    pid_t p = fork();
                    if (p == 0) {
                        char* args[] = {"./lab3", "--copy1", NULL};
                        execv("./lab3", args);
                        exit(1);
                    } else {
                        copy1_process = p;
                    }
                #endif
            }

            if (copy2_process == NULL) {
                #ifdef _WIN32
                    STARTUPINFOA si = {0}; PROCESS_INFORMATION pi = {0};
                    si.cb = sizeof(si);
                    char cmd[] = "lab3.exe --copy2";
                    CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                    copy2_process = pi.hProcess;
                    CloseHandle(pi.hThread);
                #else
                    pid_t p = fork();
                    if (p == 0) {
                        char* args[] = {"./lab3", "--copy2", NULL};
                        execv("./lab3", args);
                        exit(1);
                    } else {
                        copy2_process = p;
                    }
                #endif
            }

            last_spawn = current_time;
        }

        SLEEP_MS(10); 
    }

    return 0;
}