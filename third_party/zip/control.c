/*
 * control.c
 *
 * Functions that alter the flow of control.
 *
 */

#include "ztypes.h"

static const char *v1_lookup_table[3] = {
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    " 0123456789.,!?_#'\"/\\<-:()"
};

static const char *v3_lookup_table[3] = {
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    " \n0123456789.,!?_#'\"/\\-:()"
};

/*
 * check_argument
 *
 * Jump if argument is present.
 *
 */

#ifdef __STDC__
void check_argument (zword_t argc)
#else
void check_argument (argc)
zword_t argc;
#endif
{

    conditional_jump (argc <= (zword_t) (stack[fp + 1] & ARGS_MASK));

}/* check_argument */

/*
 * call
 *
 * Call a subroutine. Save PC and FP then load new PC and initialise stack based
 * local arguments.
 *
 */

#ifdef __STDC__
int call (int argc, zword_t *argv, int type)
#else
int call (argc, argv, type)
int argc;
zword_t *argv;
int type;
#endif
{
    zword_t arg;
    int i = 1, args, status = 0;

    /* Convert calls to 0 as returning FALSE */

    if (argv[0] == 0) {
        if (type == FUNCTION)
            store_operand (FALSE);
        return (0);
    }

    /* Save current PC, FP and argument count on stack */

    stack[--sp] = (zword_t) (pc / PAGE_SIZE);
    stack[--sp] = (zword_t) (pc % PAGE_SIZE);
    stack[--sp] = fp;
    stack[--sp] = (argc - 1) | type;

    /* Create FP for new subroutine and load new PC */

    fp = sp - 1;
    pc = (unsigned long) argv[0] * story_scaler;

    /* Read argument count and initialise local variables */

    args = (unsigned int) read_code_byte ();
    while (--args >= 0) {
        arg = (h_type > V4) ? 0 : read_code_word ();
        stack[--sp] = (--argc > 0) ? argv[i++] : arg;
    }

    /* If the call is asynchronous then call the interpreter directly.
       We will return back here when the corresponding return frame is
       encountered in the ret call. */

    if (type == ASYNC) {
        status = interpret ();
        interpreter_state = RUN;
        interpreter_status = 1;
    }

    return (status);

}/* call */

/*
 * ret
 *
 * Return from subroutine. Restore FP and PC from stack.
 *
 */

#ifdef __STDC__
void ret (zword_t value)
#else
void ret (value)
zword_t value;
#endif
{
    zword_t argc;

    /* Clean stack */

    sp = fp + 1;

    /* Restore argument count, FP and PC */

    argc = stack[sp++];
    fp = stack[sp++];
    pc = stack[sp++];
    pc += (unsigned long) stack[sp++] * PAGE_SIZE;

    /* If this was an async call then stop the interpreter and return
       the value from the async routine. This is slightly hacky using
       a global state variable, but ret can be called with conditional_jump
       which in turn can be called from all over the place, sigh. A
       better design would have all opcodes returning the status RUN, but
       this is too much work and makes the interpreter loop look ugly */

    if ((argc & TYPE_MASK) == ASYNC) {

        interpreter_state = STOP;
        interpreter_status = (int) value;

    } else {

        /* Return subroutine value for function call only */

        if ((argc & TYPE_MASK) == FUNCTION)
            store_operand (value);

    }

}/* ret */

/*
 * jump
 *
 * Unconditional jump. Jump is PC relative.
 *
 */

#ifdef __STDC__
void jump (zword_t offset)
#else
void jump (offset)
zword_t offset;
#endif
{

    pc = (unsigned long) (pc + (short) offset - 2);

}/* jump */

/*
 * restart
 *
 * Restart game by initialising environment and reloading start PC.
 *
 */

#ifdef __STDC__
void restart (void)
#else
void restart ()
#endif
{
    unsigned int i, j, restart_size, scripting_flag;

    /* Reset output buffer */

    flush_buffer (TRUE);

    /* Reset text control flags */

    formatting = ON;
    outputting = ON;
    redirecting = OFF;
    scripting_disable = OFF;

    /* Randomise */

    srand ((unsigned int) time (NULL));

    /* Remember scripting state */

    scripting_flag = get_word (H_FLAGS) & SCRIPTING_FLAG;

    /* Load restart size and reload writeable data area */

    restart_size = (h_restart_size / PAGE_SIZE) + 1;
    for (i = 0; i < restart_size; i++)
        read_page (i, &datap[i * PAGE_SIZE]);

    /* Restart the screen */

    set_status_size (0);
    set_attribute (NORMAL);
    erase_window (SCREEN);

    restart_screen ();

    /* Reset the interpreter state */

    if (scripting_flag)
        set_word (H_FLAGS, (get_word (H_FLAGS) | SCRIPTING_FLAG));

    set_byte (H_INTERPRETER, h_interpreter);
    set_byte (H_INTERPRETER_VERSION, h_interpreter_version);
    set_byte (H_SCREEN_ROWS, screen_rows); /* Screen dimension in characters */
    set_byte (H_SCREEN_COLUMNS, screen_cols);

    set_byte (H_SCREEN_LEFT, 0); /* Screen dimension in smallest addressable units, ie. pixels */
    set_byte (H_SCREEN_RIGHT, screen_cols);
    set_byte (H_SCREEN_TOP, 0);
    set_byte (H_SCREEN_BOTTOM, screen_rows);

    set_byte (H_MAX_CHAR_WIDTH, 1); /* Size of a character in screen units */
    set_byte (H_MAX_CHAR_HEIGHT, 1);

    /* Initialise status region */

    if (h_type < V4) {
        set_status_size (0);
        blank_status_line ();
    }

    /* Initialise the character translation lookup tables */

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 26; j++) {
            if (h_alternate_alphabet_offset) {
                lookup_table[i][j] = get_byte (h_alternate_alphabet_offset + (i * 26) + j);
            } else {
                if (h_type == V1)
                    lookup_table[i][j] = v1_lookup_table[i][j];
                else
                    lookup_table[i][j] = v3_lookup_table[i][j];
            }
        }
    }

    /* Load start PC, SP and FP */

    pc = h_start_pc;
    sp = STACK_SIZE;
    fp = STACK_SIZE - 1;

}/* restart */

/*
 * get_fp
 *
 * Return the value of the frame pointer (FP) for later use with unwind.
 * Before V5 games this was a simple pop.
 *
 */

#ifdef __STDC__
void get_fp (void)
#else
void get_fp ()
#endif
{

    if (h_type > V4)
        store_operand (fp);
    else
        sp++;

}/* get_fp */

/*
 * unwind
 *
 * Remove one or more stack frames and return. Works like longjmp, see get_fp.
 *
 */

#ifdef __STDC__
void unwind (zword_t value, zword_t new_fp)
#else
void unwind (value, new_fp)
zword_t value;
zword_t new_fp;
#endif
{

    if (new_fp > fp)
        fatal ("Bad frame for unwind");

    fp = new_fp;
    ret (value);

}/* unwind */
