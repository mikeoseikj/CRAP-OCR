#ifndef SEGMENT_H
#define SEGMENT_H

#include "image.h"

void threshold_image(image_t *image);
image_t *resize_image(image_t *image, int new_height, int new_width);
void remove_noise(image_t *image);

#endif