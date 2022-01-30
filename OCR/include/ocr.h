#ifndef OCR_H
#define OCR_H

#include "image.h"
typedef struct glyph
{
	image_t *image;
	int coords[4];
	int pos;
	int line;
	struct glyph *next;
}glyph_t;

glyph_t *extract_all_glyphs(image_t *image, int *avg_width);
char *template_recognition(glyph_t *extracts, int average_width);
char *neural_network_recognition(glyph_t *extracts, int average_width);
float spell_distance(char *str1, char *str2);
int is_split_char(char *str);

#endif