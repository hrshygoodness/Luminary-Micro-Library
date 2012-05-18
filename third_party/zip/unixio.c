/* unixio.c */

#include "ztypes.h"

#if !defined(BSD) || !defined(SYSTEM_FIVE) || !defined(POSIX)
#define BSD
#endif /* !defined(BSD) || !defined(SYSTEM_FIVE) || !defined(POSIX) */

#if defined(BSD)
#include <sgtty.h>
#endif /* defined(BSD) */
#if defined(SYSTEM_FIVE)
#include <termio.h>
#endif /* defined(SYSTEM_FIVE)
#if defined(POSIX)
#include <termios.h>
#endif /* defined(POSIX) */

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

static int current_row = 1;
static int current_col = 1;

static int saved_row;
static int saved_col;

static int cursor_saved = OFF;

static char tcbuf[1024];
static char cmbuf[1024];
static char *cmbufp;

static char *CE, *CL, *CM, *CS, *DL, *MD, *ME, *MR, *SE, *SO, *TE, *TI, *UE, *US;

#define GET_TC_STR(p1, p2) if ((p1 = tgetstr (p2, &cmbufp)) == NULL) p1 = ""

#define BELL 7

static void outc ();
static void display_string ();
static int wait_for_char ();
static int read_key ();
static void set_cbreak_mode ();
static void rundown ();

extern int tgetent ();
extern int tgetnum ();
extern char *tgetstr ();
extern char *tgoto ();
extern void tputs ();

static void outc (c)
int c;
{

    putchar (c);

}/* outc */

void initialize_screen ()
{
    char *term;
    int row, col;

    if ((term = getenv ("TERM")) == NULL)
        fatal ("No TERM environment variable");

    if (tgetent (tcbuf, term) <= 0)
        fatal ("No termcap entry for this terminal");

    cmbufp = cmbuf;

    GET_TC_STR (CE, "ce");
    GET_TC_STR (CL, "cl");
    GET_TC_STR (CM, "cm");
    GET_TC_STR (CS, "cs");
    GET_TC_STR (DL, "dl");
    GET_TC_STR (MD, "md");
    GET_TC_STR (ME, "me");
    GET_TC_STR (MR, "mr");
    GET_TC_STR (SE, "se");
    GET_TC_STR (SO, "so");
    GET_TC_STR (TE, "te");
    GET_TC_STR (TI, "ti");
    GET_TC_STR (UE, "ue");
    GET_TC_STR (US, "us");

    if (screen_cols == 0 && (screen_cols = tgetnum ("co")) == -1)
        screen_cols = DEFAULT_COLS;

    if (screen_rows == 0 && (screen_rows = tgetnum ("li")) == -1)
        screen_rows = DEFAULT_ROWS;

    if (*MD == '\0' || *ME == '\0' || *MR == '\0') {
        MD = SO;
        ME = SE;
        MR = SO;
    }

    if (*UE == '\0' || *US == '\0') {
        UE = SE;
        US = SO;
    }

    tputs (TI, 1, outc);

    clear_screen ();

    row = screen_rows / 2;
    col = (screen_cols - (sizeof ("The story is loading...") - 1)) / 2;
    move_cursor (row, col);
    display_string ("The story is loading...");

    h_interpreter = INTERP_MSDOS;

    set_cbreak_mode (1);

}/* initialize_screen */

void restart_screen ()
{

    cursor_saved = OFF;

    if (h_type < V4)
        set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_WINDOWS));
    else
        set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_EMPHASIS | CONFIG_WINDOWS));

    /* Force graphics off as we can't do them */

    set_word (H_FLAGS, (get_word (H_FLAGS) & (~GRAPHICS_FLAG)));

}/* restart_screen */

void reset_screen ()
{

    delete_status_window ();
    select_text_window ();
    set_attribute (NORMAL);

    set_cbreak_mode (0);

    tputs (TE, 1, outc);

}/* reset_screen */

void clear_screen ()
{

    tputs (CL, 1, outc);
    current_row = 1;
    current_col = 1;

}/* clear_screen */

void select_status_window ()
{

    save_cursor_position ();

}/* select_status_window */

void select_text_window ()
{

    restore_cursor_position ();

}/* select_text_window */

void create_status_window ()
{
    int row, col;

    if (*CS) {
        get_cursor_position (&row, &col);

        tputs (tgoto (CS, screen_rows - 1, status_size), 1, outc);

        move_cursor (row, col);
    }

}/* create_status_window */

void delete_status_window ()
{
    int row, col;

    if (*CS) {
        get_cursor_position (&row, &col);

        tputs (tgoto (CS, screen_rows - 1, 0), 1, outc);

        move_cursor (row, col);
    }

}/* delete_status_window */

void clear_line ()
{

    tputs (CE, 1, outc);

}/* clear_line */

void clear_text_window ()
{
    int i, row, col;

    get_cursor_position (&row, &col);

    for (i = status_size + 1; i <= screen_rows; i++) {
        move_cursor (i, 1);
        clear_line ();
    }

    move_cursor (row, col);

}/* clear_text_window */

void clear_status_window ()
{
    int i, row, col;

    get_cursor_position (&row, &col);

    for (i = status_size; i; i--) {
        move_cursor (i, 1);
        clear_line ();
    }

    move_cursor (row, col);

}/* clear_status_window */

void move_cursor (row, col)
int row;
int col;
{

    tputs (tgoto (CM, col - 1, row - 1), 1, outc);
    current_row = row;
    current_col = col;

}/* move_cursor */

void get_cursor_position (row, col)
int *row;
int *col;
{

    *row = current_row;
    *col = current_col;

}/* get_cursor_position */

void save_cursor_position ()
{

    if (cursor_saved == OFF) {
        get_cursor_position (&saved_row, &saved_col);
        cursor_saved = ON;
    }

}/* save_cursor_position */

void restore_cursor_position ()
{
                      
    if (cursor_saved == ON) {
        move_cursor (saved_row, saved_col);
        cursor_saved = OFF;
    }

}/* restore_cursor_position */

void set_attribute (attribute)
int attribute;
{

    if (attribute == NORMAL) {
        tputs (ME, 1, outc);
        tputs (UE, 1, outc);
    }

    if (attribute & REVERSE)
        tputs (MR, 1, outc);

    if (attribute & BOLD)
        tputs (MD, 1, outc);

    if (attribute & EMPHASIS)
        tputs (US, 1, outc);

    if (attribute & FIXED_FONT)
        ;

}/* set_attribute */

static void display_string (s)
char *s;
{

    while (*s)
        display_char (*s++);

}/* display_string */

void display_char (c)
int c;
{

    outc (c);

    if (++current_col > screen_cols)
        current_col = screen_cols;

}/* display_char */

void scroll_line ()
{
    int row, col;

    get_cursor_position (&row, &col);

    if (*CS || row < screen_rows) {
        display_char ('\n');
    } else {
        move_cursor (status_size + 1, 1);
        tputs (DL, 1, outc);
        move_cursor (row, 1);
    }

    current_col = 1;
    if (++current_row > screen_rows)
        current_row = screen_rows;

}/* scroll_line */

int input_character (timeout)
int timeout;
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday (&tv, &tz);

    tv.tv_sec += timeout;

    fflush (stdout);

    if (timeout && wait_for_char (&tv))
        return (-1);

    return (read_key ());

}/* input_character */

int input_line (buflen, buffer, timeout, read_size)
int buflen;
char *buffer;
int timeout;
int *read_size;
{
    struct timeval tv;
    struct timezone tz;
    int c, row, col;

    gettimeofday (&tv, &tz);

    tv.tv_sec += timeout;

    for ( ; ; ) {

        /* Read a single keystroke */

        fflush (stdout);

        if (timeout && wait_for_char (&tv))
            return (-1);
        c = read_key ();

        if (c == '\b') {

            /* Delete key action */

            if (*read_size == 0) {

                /* Ring bell if line is empty */

                outc (BELL);

            } else {

                /* Decrement read count */

                (*read_size)--;

                /* Erase last character typed */

                get_cursor_position (&row, &col);
                move_cursor (row, --col);
                display_char (' ');
                move_cursor (row, col);
            }

        } else {

            /* Normal key action */

            if (*read_size == (buflen - 1)) {

                /* Ring bell if buffer is full */

                outc (BELL);

            } else {

                /* Scroll line if return key pressed */

                if (c == '\n') {
                    scroll_line ();
                    return (c);
                } else {

                    /* Put key in buffer and display it */

                    buffer[(*read_size)++] = (char) c;
                    display_char (c);
                }
            }
        }
    }

}/* input_line */

static int wait_for_char (timeout)
struct timeval *timeout;
{
    int nfds, status;
    fd_set readfds;
    struct timeval tv;
    struct timezone tz;

    gettimeofday (&tv, &tz);

    if (tv.tv_sec >= timeout->tv_sec && tv.tv_usec >= timeout->tv_usec)
        return (-1);

    tv.tv_sec = timeout->tv_sec - tv.tv_sec;
    if (timeout->tv_usec < tv.tv_usec) {
        tv.tv_sec--;
        tv.tv_usec = (timeout->tv_usec + 1000000) - tv.tv_usec;
    } else
        tv.tv_usec = timeout->tv_usec - tv.tv_usec;

    nfds = getdtablesize ();
    FD_ZERO (&readfds);
    FD_SET (fileno (stdin), &readfds);

    status = select (nfds, &readfds, NULL, NULL, &tv);
    if (status < 0) {
        perror ("select");
        return (-1);
    }

    if (status == 0)
        return (-1);
    else
        return (0);

}/* wait_for_char */

static int read_key ()
{
    int c;

    c = getchar ();

    if (c == 127)
        c = '\b';
    else if (c == '\r')
        c = '\n';

    return (c);

}/* read_key */

static void set_cbreak_mode (mode)
int mode;
{
    int status;
#if defined(BSD)
    struct sgttyb new_tty;
    static struct sgttyb old_tty;
#endif /* defined(BSD) */
#if defined(SYSTEM_FIVE)
    struct termio new_termio;
    static struct termio old_termio;
#endif /* defined(SYSTEM_FIVE) */
#if defined(POSIX)
    struct termios new_termios;
    static struct termios old_termios;
#endif /* defined(POSIX) */

#if defined(BSD)
    status = ioctl (fileno (stdin), (mode) ? TIOCGETP : TIOCSETP, &old_tty);
#endif /* defined(BSD) */
#if defined(SYSTEM_FIVE)
    status = ioctl (fileno (stdin), (mode) ? TCGETA : TCSETA, &old_termio);
#endif /* defined(SYSTEM_FIVE) */
#if defined(POSIX)
    if (mode)
        status = tcgetattr (fileno (stdin), &old_termios);
    else
        status = tcsetattr (fileno (stdin), TCSANOW, &old_termios);
#endif /* defined(POSIX) */
    if (status) {
        perror ("ioctl");
        exit (1);
    }

    if (mode) {
        signal (SIGINT, rundown);
        signal (SIGTERM, rundown);
    }

    if (mode) {
#if defined(BSD)
        status = ioctl (fileno (stdin), TIOCGETP, &new_tty);
#endif /* defined(BSD) */
#if defined(SYSTEM_FIVE)
        status = ioctl (fileno (stdin), TCGETA, &new_termio);
#endif /* defined(SYSTEM_FIVE) */
#if defined(POSIX)
        status = tcgetattr (fileno (stdin), &new_termios);
#endif /* defined(POSIX) */
        if (status) {
            perror ("ioctl");
            exit (1);
        }

#if defined(BSD)
        new_tty.sg_flags |= CBREAK;
        new_tty.sg_flags &= ~ECHO;
#endif /* defined(BSD) */
#if defined(SYSTEM_FIVE)
        new_termio.c_lflag &= ~(ICANON | ECHO);
#endif /* defined(SYSTEM_FIVE) */
#if defined(POSIX)
        new_termios.c_lflag &= ~(ICANON | ECHO);
#endif /* defined(POSIX) */

#if defined(BSD)
        status = ioctl (fileno (stdin), TIOCSETP, &new_tty);
#endif /* defined(BSD) */
#if defined(SYSTEM_FIVE)
        status = ioctl (fileno (stdin), TCSETA, &new_termio);
#endif /* defined(SYSTEM_FIVE) */
#if defined(POSIX)
        status = tcsetattr (fileno (stdin), TCSANOW, &new_termios);
#endif /* defined(POSIX) */
        if (status) {
            perror ("ioctl");
            exit (1);
        }
    }

    if (mode == 0) {
        signal (SIGINT, SIG_DFL);
        signal (SIGTERM, SIG_DFL);
    }

}/* set_cbreak_mode */

static void rundown ()
{

    unload_cache ();
    close_story ();
    close_script ();
    reset_screen ();

}/* rundown */
