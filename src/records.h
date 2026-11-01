#ifndef RECORDS_H
#define RECORDS_H

#include "psx.h"

typedef struct RECORD_DATA
{
    uint32 val;
    uint8 name_state;
    uint8 name[9];
    uint8 model;
    uint8 detail;
    uint16 speed;
    uint16 peak_speed;
} RECORD_DATA;

typedef union RECORD_ENTRY
{
    RECORD_DATA data;
    uint32 words[5];
} RECORD_ENTRY;

typedef union RECORD_TABLE
{
    RECORD_ENTRY rows[47][6];
    uint32 words[1410];
} RECORD_TABLE;

extern RECORD_TABLE rec_table;

uint32 recs_reset_candidates(void);
uint32 recs_generate_defaults(void);
uint32 recs_sort(void);
uint32 recs_update_candidate(sint32 rec);
uint64 recs_set_candidate_names(void);
uint64 recs_clear_name_states(void);
uint32 recs_has_candidate(void);

#endif
