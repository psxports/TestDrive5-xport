#ifndef REPLAY_H
#define REPLAY_H

#include "psx.h"

extern uint32 replay_mode;

void replay_reset(void);
sint32 replay_exchange_input(uint32 input);

#endif
