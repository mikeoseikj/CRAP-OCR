#ifndef IMAGE_H
#define IMAGE_H

// A simple implementation of the PGM image format. Visit https://en.wikipedia.org/wiki/Netpbm for more information

#define BLACK_PIXEL   0
#define WHITE_PIXEL 255

typedef struct
{
	int width;
	int height;
	int max_color;
	unsigned char **pixels; 
}image_t;


int pgm_image_malloc(image_t **image);
image_t *pgm_image_coord_malloc(int height, int width);
image_t *pgm_image_dup(image_t *image);
void pgm_image_free(image_t *image);
int pgm_image_read(char *filename, image_t **image);
int pgm_image_write(image_t *image, char *filename);

#endif