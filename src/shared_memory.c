#include <stdio.h>
#include <stdlib.h>
#include "shared_memory.h"

#ifdef _WIN32
/* ================= WINDOWS ================= */
#include <windows.h>
#include <process.h>
#define SHM_NAME "Local\\MySharedMemory"

shared_data_t* create_shared_memory() {
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(shared_data_t), SHM_NAME);
    if (hMapFile == NULL) return NULL;

    shared_data_t* ptr = (shared_data_t*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_data_t));
    return ptr;
}

shared_data_t* attach_shared_memory() {
    HANDLE hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
    if (hMapFile == NULL) return NULL;

    shared_data_t* ptr = (shared_data_t*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_data_t));
    return ptr;
}

void destroy_shared_memory(shared_data_t* data) {
    if (data) UnmapViewOfFile(data);
}

void detach_shared_memory(shared_data_t* data) {
    if (data) UnmapViewOfFile(data);
}

void lock_memory(shared_data_t* data) {
    WaitForSingleObject(data->mutex, INFINITE);
}

void unlock_memory(shared_data_t* data) {
    ReleaseMutex(data->mutex);
}

#else
/* ================= POSIX (LINUX) ================= */
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SHM_NAME "/my_shared_memory"

shared_data_t* create_shared_memory() {
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(shared_data_t), SHM_NAME);
    if (hMapFile == NULL) return NULL;

    shared_data_t* ptr = (shared_data_t*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_data_t));
    
    if (ptr != NULL) {
        // Create the Mutex for Windows
        ptr->mutex = CreateMutexA(NULL, FALSE, "Local\\MyMutex");
    }
    return ptr;
}

shared_data_t* attach_shared_memory() {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) return NULL;
    return (shared_data_t*)mmap(0, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void destroy_shared_memory(shared_data_t* data) {
    if (data) {
        munmap(data, sizeof(shared_data_t));
        shm_unlink(SHM_NAME);
    }
}

void detach_shared_memory(shared_data_t* data) {
    if (data) munmap(data, sizeof(shared_data_t));
}

void lock_memory(shared_data_t* data) {
    pthread_mutex_lock(&data->mutex);
}

void unlock_memory(shared_data_t* data) {
    pthread_mutex_unlock(&data->mutex);
}

#endif