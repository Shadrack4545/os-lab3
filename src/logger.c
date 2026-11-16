#include <stdio.h>
#include <time.h>
#include "logger.h"

#ifdef _WIN32
    #include <windows.h>
    FILE* log_file = NULL;
    void init_logger() { log_file = fopen("process.log", "a"); }
    void write_log(int pid, int counter, const char* msg) {
        if (!log_file) return;
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(log_file, "[%02d.%02d.%04d %02d:%02d:%02d.%03d] PID:%d Counter:%d %s\n", 
                st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, pid, counter, msg);
        fflush(log_file);
    }
    void close_logger() { if (log_file) fclose(log_file); }
#else
    #include <unistd.h>
    FILE* log_file = NULL;
    void init_logger() { log_file = fopen("process.log", "a"); }
    void write_log(int pid, int counter, const char* msg) {
        if (!log_file) return;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        struct tm* tm_info = localtime(&ts.tv_sec);
        fprintf(log_file, "[%02d.%02d.%04d %02d:%02d:%02d.%03ld] PID:%d Counter:%d %s\n", 
                tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900, 
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, ts.tv_nsec / 1000000, pid, counter, msg);
        fflush(log_file);
    }
    void close_logger() { if (log_file) fclose(log_file); }
#endif