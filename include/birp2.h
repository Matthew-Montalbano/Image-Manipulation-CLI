
#ifndef BIRP2_H
#define BIRP2_H

int check_help_argument(char **argv);
int check_input_output_format(char **argv, int args_remaining);
int check_additional_args(char **argv);
int check_additional_args_with_parameter(char **argv);
void set_global_options_transformation_bits(int value);
int validate_number(char *str);
int string_to_int(char *str);
int compare_strings(char *str1, char *str2);

int write_ascii_to_output(unsigned char *raster, int width, int height, FILE *out);

unsigned char complement(unsigned char byte);
unsigned char threshold(unsigned char byte);
BDD_NODE *apply_zoom_transformation(BDD_NODE *root, int *wp, int *hp);
int negate_eight_bit_value(int value);

#endif