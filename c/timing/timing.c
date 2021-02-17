//
// Created by duncan on 17-02-21.
//
#include "timing.h"

FILE *timing_out = NULL;

void initialize_timer(char* filename) {
    timing_out = fopen(filename, "w+");
}

void finalize_timer() {
    fclose(timing_out);
}
