#ifndef LOGGER_H
#define LOGGER_H

#include "shared_memory.h"

void init_logger();
void write_log(process_id_t pid, int counter, const char* msg);
void close_logger();

#endif