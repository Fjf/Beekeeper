//
// Created by duncan on 17-02-21.
//

#ifndef HIVE_TIMING_H
#define HIVE_TIMING_H

//#include <bits/types/time_t.h>
//#include <bits/types/FILE.h>
#include <stdio.h>

#define TIMING_START 0
#define TIMING_END 1


//#define timing(name, type) { \
//    struct timespec t;       \
//    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &t); \
//    fprintf(timing_out, "%s|%s|%d|%f\n", __func__, name, type, to_usec(t)); \
//}
#define timing(name, type)

FILE *timing_out;

void initialize_timer(char* filename);
void finalize_timer();

#endif //HIVE_TIMING_H
