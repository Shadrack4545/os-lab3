#ifndef LOGGER_H
#define LOGGER_H

void init_logger();
void write_log(int pid, int counter, const char* msg);
void close_logger();

#endif