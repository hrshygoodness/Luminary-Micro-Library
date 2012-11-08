/*
 * mscio.c
 *
 * Microsoft C specific screen I/O routines for DOS.
 *
 */

#include "ztypes.h"

#include <conio.h>
#include <dos.h>
#include <graph.h>
#include <sys/timeb.h>
#include <sys/types.h>

#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define WHITE 7

#define BRIGHT 0x08
#define FLASH 0x10

static int screen_started = FALSE;
static int cursor_saved = OFF;
static int saved_row = 0;
static int saved_col = 0;
static int current_fg = WHITE;
static int current_bg = BLUE;

#ifdef __STDC__
static int timed_read_key (int, int);
static int read_key (void);
#else
static int timed_read_key ();
static int read_key ();
#endif

#ifdef __STDC__
void initialize_screen (void)
#else
void initialize_screen ()
#endif
{

    _setvideomode (_TEXTC80);

    if (screen_rows)
        screen_rows = _settextrows (screen_rows);
	if (screen_rows == 0)
		screen_rows = 25;
	screen_cols = DEFAULT_COLS;
    _settextwindow (1, 1, screen_rows, screen_cols);
    _wrapon (_GWRAPOFF);
    move_cursor (1, 1);
    set_attribute (NORMAL);
    clear_screen ();
    move_cursor (screen_rows / 2, (screen_cols - sizeof ("The story is loading...")) / 2);
    _outtext ("The story is loading...");

    screen_started = TRUE;

    h_interpreter = INTERP_MSDOS;

}/* initialize_screen */

#ifdef __STDC__
void restart_screen (void)
#else
void restart_screen ()
#endif
{

    cursor_saved = OFF;

    set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_WINDOWS));

    if (h_type > V3)
        set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_EMPHASIS | CONFIG_COLOUR));
    set_word (H_FLAGS, get_word (H_FLAGS) & (~GRAPHICS_FLAG));

}/* restart_screen */

#ifdef __STDC__
void reset_screen (void)
#else
void reset_screen ()
#endif
{

    if (screen_started == TRUE) {
        output_new_line ();
        output_string ("[Hit any key to exit.]");
        (void) input_character (0);
        output_new_line ();

        _setvideomode (_DEFAULTMODE);
    }

    screen_started = FALSE;

}/* reset_screen */

#ifdef __STDC__
void clear_screen (void)
#else
void clear_screen ()
#endif
{

    _clearscreen (_GWINDOW);

}/* clear_screen */

#ifdef __STDC__
void create_status_window (void)
#else
void create_status_window ()
#endif
{

}/* create_status_window */

#ifdef __STDC__
void delete_status_window (void)
#else
void delete_status_window ()
#endif
{

}/* delete_status_window */

#ifdef __STDC__
void select_status_window (void)
#else
void select_status_window ()
#endif
{

    save_cursor_position ();
    _displaycursor (_GCURSOROFF);

}/* select_status_window */

#ifdef __STDC__
void select_text_window (void)
#else
void select_text_window ()
#endif
{

    _displaycursor (_GCURSORON);
    restore_cursor_position ();

}/* select_text_window */

#ifdef __STDC__
void clear_line (void)
#else
void clear_line ()
#endif
{
    int i, row, col;

    get_cursor_position (&row, &col);
    for (i = col; i <= screen_rows; i++) {
        move_cursor (row, i);
        display_char (' ');
    }
    move_cursor (row, col);

}/* clear_line */

#ifdef __STDC__
void clear_text_window (void)
#else
void clear_text_window ()
#endif
{
    short left, top, right, bottom;
    int row, col;

    get_cursor_position (&row, &col);
    _gettextwindow (&top, &left, &bottom, &right);
    _settextwindow (status_size + 1, left, bottom, right);
    _clearscreen (_GWINDOW);
    _settextwindow (top, left, bottom, right);
    move_cursor (row, col);

}/* clear_text_window */

#ifdef __STDC__
void clear_status_window (void)
#else
void clear_status_window ()
#endif
{
    short left, top, right, bottom;
    int row, col;

    get_cursor_position (&row, &col);
    _gettextwindow (&top, &left, &bottom, &right);
    _settextwindow (1, left, status_size, right);
    _clearscreen (_GWINDOW);
    _settextwindow (top, left, bottom, right);
    move_cursor (row, col);

}/* clear_status_window */

#ifdef __STDC__
void move_cursor (int row, int col)
#else
void move_cursor (row, col)
int row;
int col;
#endif
{

    _settextposition (row, col);

}/* move_cursor */

#ifdef __STDC__
void get_cursor_position (int *row, int *col)
#else
void get_cursor_position (row, col)
int *row;
int *col;
#endif
{
    struct rccoord rc;

    rc = _gettextposition ();
    *row = rc.row;
    *col = rc.col;

}/* get_cursor_position */

#ifdef __STDC__
void save_cursor_position (void)
#else
void save_cursor_position ()
#endif
{

    if (cursor_saved == OFF) {
        get_cursor_position (&saved_row, &saved_col);
        cursor_saved = ON;
    }

}/* save_cursor_position */

#ifdef __STDC__
void restore_cursor_position (void)
#else
void restore_cursor_position ()
#endif
{

    if (cursor_saved == ON) {
        move_cursor (saved_row, saved_col);
        cursor_saved = OFF;
    }

}/* restore_cursor_position */

#ifdef __STDC__
void set_attribute (int attribute)
#else
void set_attribute (attribute)
int attribute;
#endif
{
    int fg, bg, new_fg, new_bg;

    fg = (int) _gettextcolor ();
    bg = (int) _getbkcolor ();

    if (attribute == NORMAL) {
        new_fg = current_fg;
        new_bg = current_bg;
    }

    if (attribute & REVERSE) {
        new_fg = bg;
        new_bg = fg;
    }

    if (attribute & BOLD) {
        new_fg = fg | BRIGHT;
        new_bg = bg;
    }

    if (attribute & EMPHASIS) {
        new_fg = RED | BRIGHT;
        new_bg = bg;
    }

    if (attribute & FIXED_FONT) {
        new_fg = fg;
        new_bg = bg;
    }

    _settextcolor ((short) new_fg);
    _setbkcolor ((long) new_bg);

}/* set_attribute */

#ifdef __STDC__
void display_char (int c)
#else
void display_char (c)
int c;
#endif
{
    char string[2];
    string[0] = (char) c;
    string[1] = '\0';
    _outtext (string);

}/* display_char */

#ifdef __STDC__
int input_line (int buflen, char *buffer, int timeout, int *read_size)
#else
int input_line (buflen, buffer, timeout, read_size)
int buflen;
char *buffer;
int timeout;
int *read_size;
#endif
{
    struct timeb timenow;
    struct tm *tmptr;
    int c, row, col, target_second, target_millisecond;

    if (timeout != 0) {
        ftime (&timenow);
        tmptr = gmtime (&timenow.time);
        target_second = (tmptr->tm_sec + timeout) % 60;
        target_millisecond = timenow.millitm;
    }

    for ( ; ; ) {

        /* Read a single keystroke */

        do {
            if (timeout == 0)
                c = read_key ();
            else {
                c = timed_read_key (target_second, target_millisecond);
                if (c == -1)
                    return (c);
            }
        } while (c == 0);

        if (c == '\b') {

            /* Delete key action */

            if (*read_size == 0) {

                /* Ring bell if line is empty */

                putchar ('\a');

            } else {

                /* Decrement read count */

                (*read_size)--;

                /* Erase last character typed */

                get_cursor_position (&row, &col);
                move_cursor (row, --col);
                _outtext (" ");
                move_cursor (row, col);
            }

        } else {

            /* Normal key action */

            if (*read_size == (buflen - 1)) {

                /* Ring bell if buffer is full */

                putchar ('\a');

            } else {

                /* Scroll line if return key pressed */

                if (c == '\r' || c == '\n') {
                    c = '\n';
                    scroll_line ();
                }

                if (c == '\n' || c >= (unsigned char) '\x080') {

                    /* Return key if it is a line terminator */

                    return ((unsigned char) c);

                } else {

                    /* Put key in buffer and display it */

                    buffer[(*read_size)++] = (char) c;
                    display_char (c);
                }
            }
        }
    }

}/* input_line */

#ifdef __STDC__
int input_character (int timeout)
#else
int input_character (timeout)
int timeout;
#endif
{
    struct timeb timenow;
    struct tm *tmptr;
    int c, target_second, target_millisecond;

    if (timeout != 0) {
        ftime (&timenow);
        tmptr = gmtime (&timenow.time);
        target_second = (tmptr->tm_sec + timeout) % 60;
        target_millisecond = timenow.millitm;
    }

    if (timeout == 0)
        c = read_key ();
    else
        c = timed_read_key (target_second, target_millisecond);

    return (c);

}/* input_character */

#ifdef __STDC__
static int timed_read_key (int target_second, int target_millisecond)
#else
static int timed_read_key (target_second, target_millisecond)
int target_second;
int target_millisecond;
#endif
{
    struct timeb timenow;
    struct tm *tmptr;
    int c;

    for ( ; ; ) {

        do {
            ftime (&timenow);
            tmptr = gmtime (&timenow.time);
        } while ((bdos (11, 0, 0) & 0xff) == 0 &&
                 (tmptr->tm_sec != target_second ||
                  (int) timenow.millitm < target_millisecond));

        if ((bdos (11, 0, 0) & 0xff) == 0)
            return (-1);
        else {
            c = read_key ();
            if (c != 0)
                return (c);
        }
    }

}/* timed_read_key */

#ifdef __STDC__
static int read_key (void)
#else
static int read_key ()
#endif
{
    int c;

    c = bdos (8, 0, 0) & 0xff;
    if (c != '\0' && c != (unsigned char) '\x0e0') {
        if (c == '\x07f')
            c = '\b';
        return (c);
    }
    c = bdos (8, 0, 0) & 0xff;
    if (c == 'H') /* Up arrow */
        return ((unsigned char) '\x081');
    else if (c == 'P') /* Down arrow */
        return ((unsigned char) '\x082');
    else if (c == 'K') /* Left arrow */
        return ((unsigned char) '\x083');
    else if (c == 'M') /* Right arrow */
        return ((unsigned char) '\x084');
    else if (c >= ';' && c <= 'D') /* Function keys F1 to F10 */
        return ((c - ';') + (unsigned char) '\x085');
    else if (c == 'O') /* End (SW) */
        return ((unsigned char) '\x092');
    else if (c == 'Q') /* PgDn (SE) */
        return ((unsigned char) '\x094');
    else if (c == 'G') /* Home (NW) */
        return ((unsigned char) '\x098');
    else if (c == 'I') /* PgUp (NE) */
        return ((unsigned char) '\x09a');

    putchar ('\a');

    return (0);

}/* read_key */

#ifdef __STDC__
void scroll_line (void)
#else
void scroll_line ()
#endif
{
    short left, top, right, bottom;
    int row, col;

    get_cursor_position (&row, &col);
    _gettextwindow (&top, &left, &bottom, &right);
    if (row == bottom) {
        _settextwindow (status_size + 1, left, bottom, right);
        _scrolltextwindow (_GSCROLLUP);
        _settextwindow (top, left, bottom, right);
    } else {
        row++;
    }
    move_cursor (row, left);

}/* scroll_line */

/*
 * set_colours
 *
 * Sets the screen foreground and background colours.
 *
 */

#ifdef __STDC__
void set_colours (int foreground, int background)
#else
void set_colours (foreground, background)
int foreground;
int background;
#endif
{
    int fg, bg;
    int colour_map[] = { BLACK, RED, GREEN, BROWN, BLUE, MAGENTA, CYAN, WHITE };

    /* Translate from Z-code colour values to natural colour values */

    fg = (foreground == 1) ? WHITE : colour_map[foreground - 2];
    bg = (background == 1) ? BLUE : colour_map[background - 2];

    /* Set foreground and background colour */

    _settextcolor ((short) fg);
    _setbkcolor ((long) bg);

    /* Save new foreground and background colours for restoring colour */

    current_fg = (int) _gettextcolor ();
    current_bg = (int) _getbkcolor ();

}/* set_colours */

/*
 * codes_to_text
 *
 * Translate Z-code characters to machine specific characters. These characters
 * include line drawing characters and international characters.
 *
 * The routine takes one of the Z-code characters from the following table and
 * writes the machine specific text replacement. The target replacement buffer
 * is defined by MAX_TEXT_SIZE in ztypes.h. The replacement text should be in a
 * normal C, zero terminated, string.
 *
 * Return 0 if a translation was available, otherwise 1.
 *
 *  Arrow characters (0x18 - 0x1b):
 *
 *  0x18 Up arrow
 *  0x19 Down arrow
 *  0x1a Right arrow
 *  0x1b Left arrow
 *
 *  International characters (0x9b - 0xa3):
 *
 *  0x9b a umlaut (ae)
 *  0x9c o umlaut (oe)
 *  0x9d u umlaut (ue)
 *  0x9e A umlaut (Ae)
 *  0x9f O umlaut (Oe)
 *  0xa0 U umlaut (Ue)
 *  0xa1 sz (ss)
 *  0xa2 open quote (>>)
 *  0xa3 close quota (<<)
 *
 *  Line drawing characters (0xb3 - 0xda):
 *
 *  0xb3 vertical line (|)
 *  0xba double vertical line (#)
 *  0xc4 horizontal line (-)
 *  0xcd double horizontal line (=)
 *  all other are corner pieces (+)
 *
 */

#ifdef __STDC__
int codes_to_text (int c, char *s)
#else
int codes_to_text (c, s)
int c;
char *s;
#endif
{

    /* Characters 24 to 27 and 179 to 218 need no translation */

    if ((c > 23 && c < 28) || (c > 178 && c < 219)) {

        s[0] = (char) c;
        s[1] = '\0';

        return (0);
    }

    /* German characters need translation */

    if (c > 154 && c < 164) {
        char xlat[9] = { 0x84, 0x94, 0x81, 0x8e, 0x99, 0x9a, 0xe1, 0xaf, 0xae };

        s[0] = xlat[c - 155];
        s[1] = '\0';

        return (0);
    }

    return (1);

}/* codes_to_text */
