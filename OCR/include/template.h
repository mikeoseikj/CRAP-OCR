#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "defines.h"

#define NUM_TMPLT_CHARS  85
#define TMPLTS_PER_CHAR  10
#define TMPLT_SIZE       IMAGE_HEIGHT*IMAGE_WIDTH
#define TMPLT_ROWS       IMAGE_HEIGHT
#define TMPLT_COLS       IMAGE_WIDTH

struct template
{	char ascii_char;
	struct 
	{
		float ratio;
		unsigned char tmplt[IMAGE_HEIGHT*IMAGE_WIDTH];
	}tmplts[TMPLTS_PER_CHAR];
};

extern struct template templates[NUM_TMPLT_CHARS];

float distance(unsigned char *tmplt1, unsigned char *tmplt2, float ratio1, float ratio2);
char recognize_template(image_t *image, int *coords);

#endif