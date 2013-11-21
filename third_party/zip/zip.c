/*
 * zip.c
 *
 * Z code interpreter main routine. Plays Infocom type 1, 2, 3, 4 and 5 games.
 *
 * Usage: zip [options] story-file-name
 *
 * options are:
 *
 *    -l n - number of lines in display
 *    -c n - number of columns in display
 *    -r n - right margin (default = 0)
 *    -t n - top margin (default = 0)
 *
 * This is a no bells and whistles Infocom interpreter for type 1 to 5 games.
 * It will automatically detect which type of game you want to play. It should
 * support all type 1 to 5 features and is based loosely on the MS-DOS version
 * with enhancements to aid portability. Read the readme.1st file for
 * information on building this program on your favourite operating system.
 * Please mail me, at the address below, if you find bugs in the code.
 *
 * Special thanks to David Doherty and Olaf Barthel for testing this program
 * and providing invaluable help and code to aid its portability.
 *
 * Mark Howell 10-Mar-93 V2.0 howell_ma@movies.enet.dec.com
 *
 * Disclaimer:
 *
 * You are expressly forbidden to use this program if in so doing you violate
 * the copyright notice supplied with the original Infocom game.
 *
 */

#include "ztypes.h"

/*
 * main
 *
 * Initialise environment, start interpreter, clean up.
 *
 */
#ifdef STANDALONE
#ifdef __STDC__
int main (int argc, char *argv[])
#else
int main (argc, argv)
int argc;
char *argv[];
#endif
{

    process_arguments (argc, argv);

    configure (V1, V5);

    initialize_screen ();

    load_cache ();

    restart ();

    (void) interpret ();

    unload_cache ();

    close_story ();

    close_script ();

    reset_screen ();

    exit (EXIT_SUCCESS);

    return (0);

}/* main */
#endif

/*
 * configure
 *
 * Initialise global and type specific variables.
 *
 */

#ifdef __STDC__
void configure (zbyte_t min_version, zbyte_t max_version)
#else
void configure (min_version, max_version)
zbyte_t min_version;
zbyte_t max_version;
#endif
{
    zbyte_t header[PAGE_SIZE];

    read_page (0, header);
    datap = header;

    h_type = get_byte (H_TYPE);

    if (h_type < min_version || h_type > max_version || (get_byte (H_CONFIG) & CONFIG_BYTE_SWAPPED))
        fatal ("wrong game or version");

    if (h_type < V4) {
        story_scaler = 2;
        story_shift = 1;
        property_mask = P3_MAX_PROPERTIES - 1;
        property_size_mask = 0xe0;
    } else {
        story_scaler = 4;
        story_shift = 2;
        property_mask = P4_MAX_PROPERTIES - 1;
        property_size_mask = 0x3f;
    }

    h_config = get_byte (H_CONFIG);
    h_version = get_word (H_VERSION);
    h_data_size = get_word (H_DATA_SIZE);
    h_start_pc = get_word (H_START_PC);
    h_words_offset = get_word (H_WORDS_OFFSET);
    h_objects_offset = get_word (H_OBJECTS_OFFSET);
    h_globals_offset = get_word (H_GLOBALS_OFFSET);
    h_restart_size = get_word (H_RESTART_SIZE);
    h_flags = get_word (H_FLAGS);
    h_synonyms_offset = get_word (H_SYNONYMS_OFFSET);
    h_file_size = get_word (H_FILE_SIZE);
    if (h_file_size == 0)
        h_file_size = get_story_size ();
    h_checksum = get_word (H_CHECKSUM);
    h_alternate_alphabet_offset = get_word (H_ALTERNATE_ALPHABET_OFFSET);

    datap = NULL;

}/* configure */
