/*
 * BIRP: Binary decision diagram Image RePresentation
 */

#include "image.h"
#include "bdd.h"
#include "const.h"
#include "debug.h"
#include "birp2.h"
#include "my_math.h"

int pgm_to_birp(FILE *in, FILE *out) {
    int temp_wp = 0;
    int temp_hp = 0;
    int *wp = &temp_wp;
    int *hp = &temp_hp;
    unsigned char *raster = raster_data;
    if (img_read_pgm(in, wp, hp, raster, RASTER_SIZE_MAX) == -1) {
        return -1;
    }
    BDD_NODE *node_pointer = bdd_from_raster(*wp, *hp, raster);
    if (node_pointer == NULL) {
        return - 1;
    }
    return img_write_birp(node_pointer, *wp, *hp, out);
}

int birp_to_pgm(FILE *in, FILE *out) {
    int temp_wp = 0;
    int temp_hp = 0;
    int *wp = &temp_wp;
    int *hp = &temp_hp;
    BDD_NODE *root = img_read_birp(in, wp, hp);
    if (root == NULL) {
        return -1;
    }
    unsigned char *raster = raster_data;
    if (((root) -> level) > 26) {
        return -1;
    }
    bdd_to_raster(root, *wp, *hp, raster);
    return img_write_pgm(raster, *wp, *hp, out);
}




int birp_to_birp(FILE *in, FILE *out) {
    int width = 0;
    int height = 0;
    int *wp = &width;
    int *hp = &height;
    BDD_NODE *root = img_read_birp(in, wp, hp);
    if (root == NULL) {
        return -1;
    }
    int transformation = global_options & 0XF00;
    transformation >>= 8;
    BDD_NODE *new_root;
    if (transformation == 0x1) {
        new_root = bdd_map(root, complement);
    } else if (transformation == 0x2) {
        new_root = bdd_map(root, threshold);
    } else if (transformation == 0x3) {
        new_root = apply_zoom_transformation(root, wp, hp);
    } else if (transformation == 0x4) {
        new_root = bdd_rotate(root, (root) -> level);
    }
    if (new_root == NULL) {
        return -1;
    }
    return img_write_birp(new_root, width, height, out);
}

unsigned char complement(unsigned char byte) {
    return 255 - byte;
}

unsigned char threshold(unsigned char byte) {
    int threshold = global_options & 0x00FF0000;
    threshold >>= 16;
    if (byte >= threshold) {
        return 255;
    } else {
        return 0;
    }
}

BDD_NODE *apply_zoom_transformation(BDD_NODE *root, int *wp, int *hp) {
    int zoom_factor = global_options & 0x00FF0000;
    zoom_factor >>= 16;
    BDD_NODE *new_root;
    int d = ((root) -> level) / 2;
    if ((zoom_factor & 0x70) == 0) { /* If zoom factor is positive */
        if (0 <= (zoom_factor + d) && (zoom_factor + d) <= BDD_LEVELS_MAX) {
            new_root = bdd_zoom(root, (root) -> level, zoom_factor);
        } else {
            return NULL;
        }
        *wp *= power(2, zoom_factor);
        *hp *= power(2, zoom_factor);
    } else {
        zoom_factor = negate_eight_bit_value(zoom_factor);
        if (0 <= (-zoom_factor + d) && (-zoom_factor + d) <= BDD_LEVELS_MAX) {
            new_root = bdd_zoom(root, (root) -> level, -zoom_factor);
        } else {
            return NULL;
        };
        int multiplier = power(2, zoom_factor);
        *wp = ceiling(((double) *wp) / ((double) multiplier));
        *hp = ceiling(((double) *hp) / ((double) multiplier));
    }
    return new_root;
}

int negate_eight_bit_value(int value) {
    value ^= 0xFF;
    return value + 1;
}

int ceiling(double value) {
    int result = (int) value;
    if ((double) result == value) {
        return result;
    }
    return result + 1;
}




int pgm_to_ascii(FILE *in, FILE *out) {
    int temp_wp = 0;
    int temp_hp = 0;
    int *wp = &temp_wp;
    int *hp = &temp_hp;
    unsigned char *raster = raster_data;
    if (img_read_pgm(in, wp, hp, raster, RASTER_SIZE_MAX) == -1) {
        return -1;
    }
    if (write_ascii_to_output(raster, *wp, *hp, out) == -1) {
        return -1;
    }
    return 0;
}

int write_ascii_to_output(unsigned char *raster, int width, int height, FILE *out) {
    unsigned char current_char;
    int row_offset;
    for (int row = 0; row < height; row++) {
        row_offset = row * width;
        for (int col = 0; col < width; col++) {
            current_char = *(raster + row_offset + col);
            if (current_char >= 0 && current_char <= 63) {
                fputc(' ', out);
            } else if (current_char >= 64 && current_char <= 127) {
                fputc('.', out);
            } else if (current_char >= 128 && current_char <= 191) {
                fputc('*', out);
            } else if (current_char >= 192 && current_char <= 255) {
                fputc('@', out);
            } else {
                return -1;
            }
        }
        fputc('\n', out);
    }
    return 0;
}





int birp_to_ascii(FILE *in, FILE *out) {
    int temp_wp = 0;
    int temp_hp = 0;
    int *wp = &temp_wp;
    int *hp = &temp_hp;
    BDD_NODE *root = img_read_birp(in, wp, hp);
    if (root == NULL) {
        return -1;
    }
    int d = ((root) -> level) / 2;
    int side_length = power(2, d);
    unsigned char *raster = raster_data;
    if (((root) -> level) > 26) {
        return -1;
    }
    bdd_to_raster(root, side_length, side_length, raster);
    return write_ascii_to_output(raster, side_length, side_length, out);
}

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specifed will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere int the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv) {
    int current_arg = 1;
    if (argc == 1 || argc > 7) { /* First argument is always the executable name */
        return -1;
    }
    argv += 1; /* Move to current arg */
    if (check_help_argument(argv)) {
        return 0;
    }
    global_options = 0x22; /* Default global_options */
    for (int i = 0; i < 2; i++) { /* Check for input/output args twice because there can be 0-2 of these args */
        int input_output_format = check_input_output_format(argv, argc - current_arg);
        if (input_output_format == 1) {
            current_arg += 2;
            argv += 2;
        } else if (input_output_format == -1) {
            return -1;
        }
        if (current_arg >= argc) {
            return 0;
        }
    }
    if ((argc - current_arg) == 2) {
        int additional_args_with_parameter = check_additional_args_with_parameter(argv);
        if (additional_args_with_parameter == -1) {
            return -1;
        }
    } else if ((argc - current_arg) == 1) {
        int additional_args = check_additional_args(argv);
        if (additional_args == -1) {
            return -1;
        }
    } else { /* If more than 2 args left by this point, command is invalid */
        return -1;
    }
    return 0;
}

int check_help_argument(char **argv) {
    if (compare_strings(*argv, "-h")) {
        global_options = 0x80000000;
        return 1;
    }
    return 0;
}

int check_input_output_format(char **argv, int args_remaining) {
    if (compare_strings(*argv, "-i")) {
        if (args_remaining < 2) {
            return -1;
        }
        argv++;
        global_options &= 0xF0; /* Mask bits 0-3 */
        if (compare_strings(*argv, "pgm")) {
            global_options |= 0x1;
        } else if (compare_strings(*argv, "birp")) {
            global_options |= 0x2;
        } else {
            return -1;
        }
    } else if (compare_strings(*argv, "-o")) {
        if (args_remaining < 2) {
            return -1;
        }
        argv++;
        global_options &= 0x0F; /* Mask bits 4-7 */
        if (compare_strings(*argv, "pgm")) {
            global_options |= 0x10;
        } else if (compare_strings(*argv, "birp")) {
            global_options |= 0x20;
        } else if (compare_strings(*argv, "ascii")) {
            global_options |= 0x30;
        } else {
            return -1;
        }
    } else {
        return 0;
    }
    return 1;
}

int check_additional_args(char **argv) {
    if (global_options != 0x22) { /* If input and output format not both 'birp' */
        return -1;
    }
    if (compare_strings(*argv, "-n")) {
        global_options |= 0x100;
    } else if (compare_strings(*argv, "-r")) {
        global_options |= 0x400;
    } else {
        return -1;
    }
    return 1;
}

int check_additional_args_with_parameter(char **argv) {
    if (global_options != 0x22) { /* If input and output format not both 'birp' */
        return -1;
    }
    if (compare_strings(*argv, "-t")) {
        argv++;
        if (!validate_number(*argv)) {
            return -1;
        }
        int threshold = string_to_int(*argv);
        if (threshold >= 0 && threshold <= 255) {
            global_options |= 0x200;
            set_global_options_transformation_bits(threshold);
        } else {
            return -1;
        }
    } else if (compare_strings(*argv, "-z")) {
        argv++;
        if (!validate_number(*argv)) {
            return -1;
        }
        int zoom = string_to_int(*argv);
        if (zoom >= 0 && zoom <= 16) {
            global_options |= 0x300;
            set_global_options_transformation_bits(-zoom);
        } else {
            return -1;
        }
    } else if (compare_strings(*argv, "-Z")) {
        argv++;
        if (!validate_number(*argv)) {
            return -1;
        }
        int zoom = string_to_int(*argv);
        if (zoom >= 0 && zoom <= 16) {
            global_options |= 0x300;
            set_global_options_transformation_bits(zoom);
        } else {
            return -1;
        }
    } else {
        return -1;
    }
    return 1;
}

void set_global_options_transformation_bits(int value) {
    value <<= 16;
    value &= 0x00FF0000; /* Mask so only bits 16-23 are relevant */
    global_options &= 0xFF00FFFF; /* Mask to make room for transformation information */
    global_options |= value;
}

int validate_number(char *str) {
    while (*str != '\0') {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        str++;
    }
    return 1;
}

int string_to_int(char *str) {
    int result = 0;
    while (*str != '\0') { /* While string is not at the null terminator */
        result *= 10;
        result += (*str - '0');
        str++;
    }
    return result;
}

int compare_strings(char *str1, char *str2) {
    while (*str1 != '\0' && *str2 != '\0') {
        if (*str1 != *str2) {
            return 0;
        }
        str1++;
        str2++;
    }
    if (*str1 == '\0' && *str2 == '\0') {
        return 1;
    }
    return 0;
}
