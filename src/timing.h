#ifndef TIMING_H
#define TIMING_H

#include "psx.h"

sint32 timing_reset_counter(sint32 timer);

sint32 timing_read_counter(sint32 timer, sint32 prev_result);

#endif
