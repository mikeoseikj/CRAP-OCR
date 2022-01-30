#ifndef UTILS_H
#define UTILS_H

#include "image.h"

#define TOP_ROW   0
#define DOWN_ROW  1
#define LEFT_COL  2
#define RIGHT_COL 3


#define NUM_OF_LETTERS    26
#define MAX_WORD_LEN     255	

#define REGARD_LENGTH      1	// this flag is only useful in this program. Using it outside of this program will make spell_check() perform poorly
#define DISREGARD_LENGTH   2

float *alloc_float_array(int size);
void get_char_enclose_box(image_t *image, int i, int j, int *coords);
void image_to_nn_input(float *output, image_t *image);
int digits_in_word(char *str);
void strtolower(char *str);
char *spell_check(char *word, int mode);

#endif