#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#ifdef _WIN32
    #include <windows.h>
    typedef HANDLE mutex_t;
#else
    #include <pthread.h>
    #include <semaphore.h>
    typedef pthread_mutex_t mutex_t;
#endif

// The structure that will be shared between processes
typedef struct {
    int counter;
    pid_t master_pid; // To identify who is the "master" program (Req #6)
    mutex_t mutex;    // The lock to prevent race conditions
} shared_data_t;

// Functions to manage shared memory
shared_data_t* create_shared_memory();
shared_data_t* attach_shared_memory();
void destroy_shared_memory(shared_data_t* data);
void detach_shared_memory(shared_data_t* data);

// Functions for locking
void lock_memory(shared_data_t* data);
void unlock_memory(shared_data_t* data);

#endif