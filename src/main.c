#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options & HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    int input_output = global_options & 0x000000FF;
    int exit_status = -1;
    if (input_output == 0x31) {
    	exit_status = pgm_to_ascii(stdin, stdout);
    } else if (input_output == 0x21) {
    	exit_status = pgm_to_birp(stdin, stdout);
    } else if (input_output == 0x12) {
    	exit_status = birp_to_pgm(stdin, stdout);
    } else if (input_output == 0x32) {
    	exit_status = birp_to_ascii(stdin, stdout);
    } else if (input_output == 0x22) {
    	exit_status = birp_to_birp(stdin, stdout);
    }
    if (exit_status == 0) {
    	return EXIT_SUCCESS;
    } else {
    	error();
    	return EXIT_FAILURE;
    }
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
