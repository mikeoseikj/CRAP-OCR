#ifndef SKEW_H
#define SKEW_H

#include "image.h"

image_t *rotate_image(image_t *image, float angle);
image_t *deskew(image_t *image, int *skew_angle);

#endif