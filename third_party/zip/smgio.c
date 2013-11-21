/* smgio.c */

#include <descrip.h>
#include <iodef.h>
#include <smg$routines.h>
#include <smgdef.h>
#include <smgmsg.h>
#include <ssdef.h>
#include <str$routines.h>

#include "ztypes.h"

#define DESC_DECL_S(p1) struct dsc$descriptor_s  p1
#define DESC_INIT_S(p1, p2, p3) \
    p1.dsc$w_length = (p2), \
    p1.dsc$b_dtype = DSC$K_DTYPE_T, \
    p1.dsc$b_class = DSC$K_CLASS_S, \
    p1.dsc$a_pointer = (char *) (p3)

#define DESC_DECL_D(p1) struct dsc$descriptor_d  p1
#define DESC_INIT_D(p1, p2, p3) \
    p1.dsc$w_length = (p2), \
    p1.dsc$b_dtype = DSC$K_DTYPE_T, \
    p1.dsc$b_class = DSC$K_CLASS_D, \
    p1.dsc$a_pointer = (char *) (p3)

static int pasteboard_id = 0;
static int display_id;
static int keyboard_id;
static int keytable_id;
static int current_attr = SMG$M_NORMAL;

static int saved_row;
static int saved_col;
static int cursor_saved = OFF;

static void flush (void);
static void set_keys (void);
static unsigned short translate_key (unsigned short);

void initialize_screen (void)
{
    int attr, row, col;
    DESC_DECL_S (text);

    smg$create_pasteboard (&pasteboard_id, NULL, &screen_rows, &screen_cols);
    smg$create_virtual_display (&screen_rows, &screen_cols, &display_id);
    smg$create_virtual_keyboard (&keyboard_id);
    smg$create_key_table (&keytable_id);

    attr = SMG$M_KEYPAD_APPLICATION;
    smg$set_keypad_mode (&keytable_id, &attr);

    set_keys ();

    row = 1;
    col = 1;
    smg$paste_virtual_display (&display_id, &pasteboard_id, &row, &col);

    row = screen_rows / 2;
    col = (screen_cols - (sizeof ("The story is loading...") - 1)) / 2;
    DESC_INIT_S (text, sizeof ("The story is loading...") - 1, "The story is loading...");
    smg$put_chars (&display_id, &text, &row, &col);

    h_interpreter = INTERP_MSDOS;

    smg$begin_display_update (&display_id);

}/* initialize_screen */

void restart_screen (void)
{

    cursor_saved = OFF;

    if (h_type < V4)
        set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_WINDOWS));
    else
        set_byte (H_CONFIG, (get_byte (H_CONFIG) | CONFIG_EMPHASIS | CONFIG_WINDOWS));

    /* Force graphics off as we can't do them */

    set_word (H_FLAGS, (get_word (H_FLAGS) & (~GRAPHICS_FLAG)));

    smg$begin_display_update (&display_id);

}/* restart_screen */

void reset_screen (void)
{
    int attr;

    if (pasteboard_id == 0)
        return;

    output_new_line ();
    output_string ("[Hit any key to exit.]");
    (void) input_character (0);
    output_new_line ();

    delete_status_window ();
    select_text_window ();
    set_attribute (NORMAL);

    attr = 0;
    smg$set_keypad_mode (&keytable_id, &attr);

    smg$delete_virtual_keyboard (&keyboard_id);
    smg$delete_virtual_display (&display_id);
    smg$delete_pasteboard (&pasteboard_id);

}/* reset_screen */

void clear_screen (void)
{

    smg$erase_display (&display_id);

}/* clear_screen */

void select_status_window (void)
{
    unsigned long int flags;

    flush ();

    save_cursor_position ();
#if 0
    flags = SMG$M_CURSOR_OFF;
    smg$set_cursor_mode (&pasteboard_id, &flags);
#endif

    smg$begin_display_update (&display_id);

}/* select_status_window */

void select_text_window (void)
{
    unsigned long int flags;

    flush ();

#if 0
    flags = SMG$M_CURSOR_ON;
    smg$set_cursor_mode (&pasteboard_id, &flags);
#endif
    restore_cursor_position ();

    smg$begin_display_update (&display_id);

}/* select_text_window */

void create_status_window (void)
{
    unsigned long int start;
    int row, col;

    get_cursor_position (&row, &col);

    start = status_size + 1;
    smg$set_display_scroll_region (&display_id, &start, &screen_rows);

    move_cursor (row, col);

}/* create_status_window */

void delete_status_window (void)
{

    smg$set_display_scroll_region (&display_id);

}/* delete_status_window */

void clear_line (void)
{

    smg$erase_line (&display_id);

}/* clear_line */

void clear_text_window (void)
{
    int row, col;
    int ur, uc, lr, lc;

    get_cursor_position (&row, &col);

    ur = status_size + 1;
    uc = 1;
    lr = screen_rows;
    lc = screen_cols;
    smg$erase_display (&display_id, &ur, &uc, &lr, &lc);

    move_cursor (row, col);

}/* clear_text_window */

void clear_status_window (void)
{
    int row, col;
    int ur, uc, lr, lc;

    get_cursor_position (&row, &col);

    ur = 1;
    uc = 1;
    lr = status_size;
    lc = screen_cols;
    smg$erase_display (&display_id, &ur, &uc, &lr, &lc);

    move_cursor (row, col);

}/* clear_status_window */

void move_cursor (int row, int col)
{

    smg$set_cursor_abs (&display_id, &row, &col);

}/* move_cursor */

void get_cursor_position (int *row, int *column)
{

    smg$return_cursor_pos (&display_id, row, column);

}/* get_cursor_position */

void save_cursor_position (void)
{

    if (cursor_saved == OFF) {
        get_cursor_position (&saved_row, &saved_col);
        cursor_saved = ON;
    }

}/* save_cursor_position */

void restore_cursor_position (void)
{
                      
    if (cursor_saved == ON) {
        move_cursor (saved_row, saved_col);
        cursor_saved = OFF;
    }

}/* restore_cursor_position */

void set_attribute (int attribute)
{

    if (attribute == NORMAL)
        current_attr = SMG$M_NORMAL;

    if (attribute & REVERSE)
        current_attr |= SMG$M_REVERSE;

    if (attribute & BOLD)
        current_attr |= SMG$M_BOLD;

    if (attribute & EMPHASIS)
        current_attr |= SMG$M_UNDERLINE;

    if (attribute & FIXED_FONT)
        current_attr = current_attr;

}/* set_attribute */

void display_char (int c)
{
    DESC_DECL_S (text);

    if (c == '\n')
        DESC_INIT_S (text, sizeof ("\r\n") - 1, "\r\n");
    else
        DESC_INIT_S (text, 1, &c);
    smg$put_chars (&display_id, &text, NULL, NULL, NULL, &current_attr);

}/* display_char */

void scroll_line (void)
{

    flush ();

    display_char ('\n');

    smg$begin_display_update (&display_id);

}/* scroll_line */

static void flush (void)
{

    while (smg$end_display_update (&display_id) == SMG$_BATSTIPRO)
        ;

}

int input_character (int timeout)
{
    unsigned long int status;
    unsigned short c;

    flush ();

    do {

        status = smg$read_keystroke (&keyboard_id, &c, NULL, (timeout) ? &timeout : NULL);

        if (status == SS$_TIMEOUT)
            return (-1);

        if (c == SMG$K_TRM_CTRLW)
            smg$repaint_screen (&pasteboard_id);

    } while (c == SMG$K_TRM_CTRLW);

    smg$begin_display_update (&display_id);

    c = translate_key (c);

    return (c);

}/* input_character */

int input_line (int buflen, char *buffer, int timeout, int *read_size)
{
    unsigned long status;
    unsigned short terminator;
    int i, row, col;
    DESC_DECL_S (initial_text);
    DESC_DECL_D (text);

    flush ();

    do {

        if (*read_size != 0) {
            get_cursor_position (&row, &col);
            move_cursor (row, col - *read_size);
        }

        DESC_INIT_S (initial_text, *read_size, buffer);
        DESC_INIT_D (text, 0, NULL);

        str$copy_dx (&text, &initial_text);

        status = smg$read_composed_line (&keyboard_id, &keytable_id,
                                         &text, NULL, NULL,
                                         &display_id, NULL,
                                         &initial_text,
                                         (timeout) ? &timeout : NULL, NULL, NULL,
                                         &terminator);

        *read_size = text.dsc$w_length;

        if (*read_size != 0 && (unsigned char) text.dsc$a_pointer[*read_size - 1] > 127)
            terminator = (unsigned char) text.dsc$a_pointer[--(*read_size)];

        for (i = 0; i < text.dsc$w_length; i++)
            buffer[i] = translate_key ((unsigned char) text.dsc$a_pointer[i]);

        str$free1_dx (&text);

        if (status == SS$_TIMEOUT)
            return (-1);

        if (terminator == SMG$K_TRM_CTRLW)
            smg$repaint_screen (&pasteboard_id);

    } while (terminator == SMG$K_TRM_CTRLW);

    get_cursor_position (&row, &col);
#if 0
    if (row == screen_rows || (*read_size == 0 && terminator == '\n')) /* Due to a bug in SMG */
#else
    if (row == screen_rows || *read_size == 0) /* Due to a bug in SMG */
#endif
        display_char ('\n');

    smg$begin_display_update (&display_id);

    return (translate_key (terminator));

}/* input_line */

static void set_keys (void)
{
    unsigned long attr;
    DESC_DECL_S (key);
    DESC_DECL_S (text);

    attr = SMG$M_KEY_TERMINATE | SMG$M_KEY_NOECHO;

    /* Refresh screen key */

    DESC_INIT_S (key, sizeof ("CTRLW") - 1, "CTRLW");
    smg$add_key_def (&keytable_id, &key, NULL, &attr);

    /* Function keys 1-10, map to F17-F20, PF1-PF4, KP- and KP, */

    DESC_INIT_S (key, sizeof ("F17") - 1, "F17");
    DESC_INIT_S (text, 1, "\x85");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("F18") - 1, "F18");
    DESC_INIT_S (text, 1, "\x86");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("F19") - 1, "F19");
    DESC_INIT_S (text, 1, "\x87");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("F20") - 1, "F20");
    DESC_INIT_S (text, 1, "\x88");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("PF1") - 1, "PF1");
    DESC_INIT_S (text, 1, "\x89");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("PF2") - 1, "PF2");
    DESC_INIT_S (text, 1, "\x8a");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("PF3") - 1, "PF3");
    DESC_INIT_S (text, 1, "\x8b");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("PF4") - 1, "PF4");
    DESC_INIT_S (text, 1, "\x8c");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("MINUS") - 1, "MINUS");
    DESC_INIT_S (text, 1, "\x8d");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("COMMA") - 1, "COMMA");
    DESC_INIT_S (text, 1, "\x8e");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    /* Keypad keys */

    DESC_INIT_S (key, sizeof ("KP1") - 1, "KP1");
    DESC_INIT_S (text, 1, "\x92");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP2") - 1, "KP2");
    DESC_INIT_S (text, 1, "\x93");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP3") - 1, "KP3");
    DESC_INIT_S (text, 1, "\x94");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP4") - 1, "KP4");
    DESC_INIT_S (text, 1, "\x95");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP5") - 1, "KP5");
    DESC_INIT_S (text, 1, "\x96");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP6") - 1, "KP6");
    DESC_INIT_S (text, 1, "\x97");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP7") - 1, "KP7");
    DESC_INIT_S (text, 1, "\x98");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP8") - 1, "KP8");
    DESC_INIT_S (text, 1, "\x99");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

    DESC_INIT_S (key, sizeof ("KP9") - 1, "KP9");
    DESC_INIT_S (text, 1, "\x9a");
    smg$add_key_def (&keytable_id, &key, NULL, &attr, &text);

}/* set_keys */

static unsigned short translate_key (unsigned short c)
{
    unsigned char tc;

    if (c == SMG$K_TRM_UP)
        tc = '\x81';
    else if (c == SMG$K_TRM_DOWN)
        tc = '\x82';
    else if (c == SMG$K_TRM_LEFT)
        tc = '\x83';
    else if (c == SMG$K_TRM_RIGHT)
        tc = '\x84';
    else if (c == SMG$K_TRM_F11)
        tc = '\x85';
    else if (c == SMG$K_TRM_F12)
        tc = '\x86';
    else if (c == SMG$K_TRM_F13)
        tc = '\x87';
    else if (c == SMG$K_TRM_F14)
        tc = '\x88';
    else if (c == SMG$K_TRM_F15)
        tc = '\x89';
    else if (c == SMG$K_TRM_F16)
        tc = '\x8a';
    else if (c == SMG$K_TRM_F17)
        tc = '\x8b';
    else if (c == SMG$K_TRM_F18)
        tc = '\x8c';
    else if (c == SMG$K_TRM_F19)
        tc = '\x8d';
    else if (c == SMG$K_TRM_F20)
        tc = '\x8e';
    else if (c == SMG$K_TRM_KP1)
        tc = '\x92';
    else if (c == SMG$K_TRM_KP2)
        tc = '\x93';
    else if (c == SMG$K_TRM_KP3)
        tc = '\x94';
    else if (c == SMG$K_TRM_KP4)
        tc = '\x95';
    else if (c == SMG$K_TRM_KP5)
        tc = '\x96';
    else if (c == SMG$K_TRM_KP6)
        tc = '\x97';
    else if (c == SMG$K_TRM_KP7)
        tc = '\x98';
    else if (c == SMG$K_TRM_KP8)
        tc = '\x99';
    else if (c == SMG$K_TRM_KP9)
        tc = '\x9a';
    else if (c == 0xe4)
        tc = '\x9b';
    else if (c == 0xf6)
        tc = '\x9c';
    else if (c == 0xfc)
        tc = '\x9d';
    else if (c == 0xc4)
        tc = '\x9e';
    else if (c == 0xd6)
        tc = '\x9f';
    else if (c == 0xdc)
        tc = '\xa0';
    else if (c == 0xdf)
        tc = '\xa1';
    else if (c == 0xbb)
        tc = '\xa2';
    else if (c == 0xab)
        tc = '\xa3';
    else
        tc = c;

    return (tc);

}/* translate_key */

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

    /* German characters need translation */

    if (c > 154 && c < 164) {
        char xlat[9] = { 0xe4, 0xf6, 0xfc, 0xc4, 0xd6, 0xdc, 0xdf, 0xbb, 0xab };

        s[0] = xlat[c - 155];
        s[1] = '\0';

        return (0);
    }

    return (1);

}/* codes_to_text */
