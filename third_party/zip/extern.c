/*
 * extern.c
 *
 * Global data.
 *
 */

#include "ztypes.h"

/* Game header data */

zbyte_t h_type = 0;
zbyte_t h_config = 0;
zword_t h_version = 0;
zword_t h_data_size = 0;
zword_t h_start_pc = 0;
zword_t h_words_offset = 0;
zword_t h_objects_offset = 0;
zword_t h_globals_offset = 0;
zword_t h_restart_size = 0;
zword_t h_flags = 0;
zword_t h_synonyms_offset = 0;
zword_t h_file_size = 0;
zword_t h_checksum = 0;
zbyte_t h_interpreter = INTERP_MSDOS;
zbyte_t h_interpreter_version = 'B'; /* Interpreter version 2 */
zword_t h_alternate_alphabet_offset = 0;

/* Game version specific data */

int story_scaler = 0;
int story_shift = 0;
int property_mask = 0;
int property_size_mask = 0;

/* Stack and PC data */

zword_t stack[STACK_SIZE];
zword_t sp = STACK_SIZE;
zword_t fp = STACK_SIZE - 1;
unsigned long pc = 0;
int interpreter_state = RUN;
int interpreter_status = 0;

/* Data region data */

unsigned int data_size = 0;
zbyte_t *datap = NULL;
zbyte_t *undo_datap = NULL;

/* Screen size data */

int screen_rows = 0;
int screen_cols = 0;
int right_margin = DEFAULT_RIGHT_MARGIN;
int top_margin = DEFAULT_TOP_MARGIN;

/* Current window data */

int screen_window = TEXT_WINDOW;

/* Formatting and output control data */

int formatting = ON;
int outputting = ON;
int redirecting = OFF;
int scripting_disable = OFF;
int scripting = OFF;
int recording = OFF;
int replaying = OFF;
int font = 1;

/* Status region data */

int status_active = OFF;
int status_size = 0;

/* Text output buffer data */

int lines_written = 0;
int status_pos = 0;

/* Dynamic buffer data */

char *line = NULL;
char *status_line = NULL;

/* Character translation tables */

char lookup_table[3][26];
