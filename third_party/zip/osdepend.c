/*
 * osdepend.c
 *
 * All non screen specific operating system dependent routines.
 *
 * Olaf Barthel 28-Jul-1992
 *
 */

#include "ztypes.h"

/* File names will be O/S dependent */

#if defined(AMIGA)
#define SAVE_NAME   "Story.Save"   /* Default save name */
#define SCRIPT_NAME "PRT:"         /* Default script name */
#define RECORD_NAME "Story.Record" /* Default record name */
#else /* defined(AMIGA) */
#define SAVE_NAME   "story.sav"   /* Default save name */
#define SCRIPT_NAME "story.lis"   /* Default script name */
#define RECORD_NAME "record.lis"  /* Default record name */
#endif /* defined(AMIGA) */

#if !defined(AMIGA)

/* getopt linkages */

extern int optind;
extern const char *optarg;

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * process_arguments
 *
 * Do any argument preprocessing necessary before the game is
 * started. This may include selecting a specific game file or
 * setting interface-specific options.
 *
 */

#ifdef __STDC__
void process_arguments (int argc, char *argv[])
#else
void process_arguments (argc, argv)
int argc;
char *argv[];
#endif
{
    int c, errflg = 0;

    /* Parse the options */

    while ((c = getopt (argc, argv, "hl:c:r:t:")) != EOF) {
        switch (c) {
            case 'l': /* lines */
                screen_rows = atoi (optarg);
                break;
            case 'c': /* columns */
                screen_cols = atoi (optarg);
                break;
            case 'r': /* right margin */
                right_margin = atoi (optarg);
                break;
            case 't': /* top margin */
                top_margin = atoi (optarg);
                break;
            case 'h':
            case '?':
            default:
                errflg++;
        }
    }

    /* Display usage */

    if (errflg || optind >= argc) {
        fprintf (stderr, "usage: %s [options...] story-file\n\n", argv[0]);
        fprintf (stderr, "ZIP - an Infocom story file interpreter. Version 2.0 by Mark Howell\n");
        fprintf (stderr, "Plays type 1 to 5 Infocom games.\n\n");
        fprintf (stderr, "\t-l n lines in display\n");
        fprintf (stderr, "\t-c n columns in display\n");
        fprintf (stderr, "\t-r n text right margin (default = %d)\n", DEFAULT_RIGHT_MARGIN);
        fprintf (stderr, "\t-t n text top margin (default = %d)\n", DEFAULT_TOP_MARGIN);
        exit (EXIT_FAILURE);
    }

    /* Open the story file */

    open_story (argv[optind]);

}/* process_arguments */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * file_cleanup
 *
 * Perform actions when a file is successfully closed. Flag can be one of:
 * GAME_SAVE, GAME_RESTORE, GAME_SCRIPT.
 *
 */

#ifdef __STDC__
void file_cleanup (const char *file_name, int flag)
#else
void file_cleanup (file_name, flag)
const char *file_name;
int flag;
#endif
{

}/* file_cleanup */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * sound
 *
 * Play a sound file or a note.
 *
 * argc = 1: argv[0] = note# (range 1 - 3)
 *
 *           Play note.
 *
 * argc = 2: argv[0] = 0
 *           argv[1] = 3
 *
 *           Stop playing current sound.
 *
 * argc = 2: argv[0] = 0
 *           argv[1] = 4
 *
 *           Free allocated resources.
 *
 * argc = 3: argv[0] = ID# of sound file to replay.
 *           argv[1] = 2
 *           argv[2] = Volume to replay sound with, this value
 *                     can range between 1 and 8.
 *
 * argc = 4: argv[0] = ID# of sound file to replay.
 *           argv[1] = 2
 *           argv[2] = Control information
 *           argv[3] = Volume information
 *
 *           Volume information:
 *
 *               0x34FB -> Fade sound in
 *               0x3507 -> Fade sound out
 *               other  -> Replay sound at maximum volume
 *
 *           Control information:
 *
 *               This word is divided into two bytes,
 *               the upper byte determines the number of
 *               cycles to play the sound (e.g. how many
 *               times a clock chimes or a dog barks).
 *               The meaning of the lower byte is yet to
 *               be discovered :)
 *
 */

#ifdef __STDC__
void sound (int argc, zword_t *argv)
#else
void sound (argc, argv)
int argc;
zword_t *argv;
#endif
{

    /* Supply default parameters */

    if (argc < 4)
        argv[3] = 0;
    if (argc < 3)
        argv[2] = 0xff;
    if (argc < 2)
        argv[1] = 2;

    /* Generic bell sounder */

    if (argc == 1 || argv[1] == 2)
        display_char ('\007');

}/* sound */

#endif /* !defined(AMIGA) */

/*
 * get_file_name
 *
 * Return the name of a file. Flag can be one of:
 *    GAME_SAVE    - Save file (write only)
 *    GAME_RESTORE - Save file (read only)
 *    GAME_SCRIPT  - Script file (write only)
 *    GAME_RECORD  - Keystroke record file (write only)
 *    GAME_PLABACK - Keystroke record file (read only)
 *
 */

#ifdef __STDC__
int get_file_name (char *file_name, char *default_name, int flag)
#else
int get_file_name (file_name, default_name, flag)
char *file_name;
char *default_name;
int flag;
#endif
{
    char buffer[127 + 2]; /* 127 is the biggest positive char */
    int status = 0;

    /* If no default file name then supply the standard name */

    if (default_name[0] == '\0') {
        if (flag == GAME_SCRIPT)
            strcpy (default_name, SCRIPT_NAME);
        else if (flag == GAME_RECORD || flag == GAME_PLAYBACK)
            strcpy (default_name, RECORD_NAME);
        else /* (flag == GAME_SAVE || flag == GAME_RESTORE) */
            strcpy (default_name, SAVE_NAME);
    }

    /* Prompt for the file name */

    output_line ("Enter a file name.");
    output_string ("(Default is \"");
    output_string (default_name);
    output_string ("\"): ");

    buffer[0] = 127;
    buffer[1] = 0;
    (void) get_line (buffer, 0, 0);

    /* Copy file name from the input buffer */

    strcpy (file_name, (h_type > V4) ? &buffer[2] : &buffer[1]);

    /* If nothing typed then use the default name */

    if (file_name[0] == '\0')
        strcpy (file_name, default_name);

#if !defined(VMS) /* VMS has file version numbers, so cannot overwrite */

    /* Check if we are going to overwrite the file */

    if (flag == GAME_SAVE || flag == GAME_SCRIPT || flag == GAME_RECORD) {
        FILE *tfp;
        char c;

        /* Try to access the file */

        tfp = fopen (file_name, "r");
        if (tfp != NULL) {
            fclose (tfp);

            /* If it succeeded then prompt to overwrite */

            output_line ("You are about to write over an existing file.");
            output_string ("Proceed? (Y/N) ");

            do {
                c = (char) input_character (0);
                c = (char) toupper (c);
            } while (c != 'Y' && c != 'N');

            output_char (c);
            output_new_line ();

            /* If no overwrite then fail the routine */

            if (c == 'N')
                status = 1;
        }
    }
#endif /* !defined(VMS) */

    /* Record the file name if it was OK */

    if (status == 0)
        record_line (file_name);

    return (status);

}/* get_file_name */

#if !defined(AMIGA)

/*
 * fatal
 *
 * Display message and stop interpreter.
 *
 */

#ifdef __STDC__
void fatal (const char *s)
#else
void fatal (s)
const char *s;
#endif
{

    reset_screen ();
    printf ("\nFatal error: %s (PC = %lx)\n", s, pc);
    exit (1);

}/* fatal */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * fit_line
 *
 * This routine determines whether a line of text will still fit
 * on the screen.
 *
 * line : Line of text to test.
 * pos  : Length of text line (in characters).
 * max  : Maximum number of characters to fit on the screen.
 *
 */

#ifdef __STDC__
int fit_line (const char *line_buffer, int pos, int max)
#else
int fit_line (line_buffer, pos, max)
const char *line_buffer;
int pos;
int max;
#endif
{

    return (pos < max);

}/* fit_line */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * print_status
 *
 * Print the status line (type 3 games only).
 *
 * argv[0] : Location name
 * argv[1] : Moves/Time
 * argv[2] : Score
 *
 * Depending on how many arguments are passed to this routine
 * it is to print the status line. The rendering attributes
 * and the status line window will be have been activated
 * when this routine is called. It is to return FALSE if it
 * cannot render the status line in which case the interpreter
 * will use display_char() to render it on its own.
 *
 * This routine has been provided in order to support
 * proportional-spaced fonts.
 *
 */

#ifdef __STDC__
int print_status (int argc, char *argv[])
#else
int print_status (argc, argv)
int argc;
char *argv[];
#endif
{

    return (FALSE);

}/* print_status */

#endif /* !defined(AMIGA) */

#if !defined(AMIGA)

/*
 * set_font
 *
 * Set a new character font. Font can be either be:
 *
 *    TEXT_FONT (1)     = normal text character font
 *    GRAPHICS_FONT (3) = graphical character font
 *
 */

#ifdef __STDC__
void set_font (int font_type)
#else
void set_font (font_type)
int font_type;
#endif
{

}/* set_font */

#endif /* !defined(AMIGA) */

#if !defined(MSDOS) && !defined(AMIGA)

/*
 * set_colours
 *
 * Sets screen foreground and background colours.
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

}/* set_colours */

#endif /* !defined(MSDOS) && !defined(AMIGA) */

#if !defined(VMS) && !defined(MSDOS)

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

}/* codes_to_text */

#endif /* !defined(VMS) && !defined(MSDOS) */
