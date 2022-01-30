#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "include/image.h"

image_t *rotate_image(image_t *image, float angle)
{
	angle = -(angle*M_PI)/180.0;	// Negated in order to do rotations in the clockwise direction
	image_t *rotated = pgm_image_coord_malloc(image->height, image->width);
	if(rotated == NULL)
		return NULL;
	rotated->max_color = image->max_color;

	for(int i = 0; i < rotated->height; i++)
	{
		for(int j = 0; j < rotated->width; j++)
			rotated->pixels[i][j] = WHITE_PIXEL;
	}

	float x0 = image->width/2 - cos(angle)*rotated->width/2 - sin(angle)*rotated->height/2;
 	float y0 = image->height/2 - cos(angle)*rotated->height/2 + sin(angle)*rotated->width/2;
	for(int x = 0; x < rotated->height; x++)
	{
		for(int y = 0; y < rotated->width; y++)
		{
			int src_x = x*cos(angle)  + y*sin(angle) + x0;
			int src_y = x*-sin(angle) + y*cos(angle) + y0;
			if(src_x >= 0 && src_x < image->height && src_y >= 0 && src_y < image->width)
				rotated->pixels[x][y] = image->pixels[src_x][src_y];
		}
	}	
	return rotated;
}


static int *horizontal_histogram(image_t *image)
{
	int *histogram = malloc(sizeof(int)*image->height);
	if(histogram == NULL)
		return NULL;

	for(int i = 0; i < image->height; i++)
	{
		histogram[i] = 0;
		for(int j = 0; j < image->width; j++)
		{
			if(image->pixels[i][j] == BLACK_PIXEL)
				histogram[i]++;
		}
	}
	return histogram;
}

static int max(int *histogram, int len)
{
	if(len <= 0)
		return 0;

	int val = histogram[0];
	for(int i = 1; i < len; i++)
	{
		if(histogram[i] > val)
			val = histogram[i];
	}
	return val;
}


image_t *deskew(image_t *image, int *skew_angle)
{
	*skew_angle = 0;
	int *histogram = horizontal_histogram(image);
	if(histogram == NULL)
		return NULL;

	int max_val = max(histogram, image->height), curr_angle = 0;
	free(histogram);
	image_t *rotated;
	for(int angle = 1; angle < 360; angle++)
	{
		rotated = rotate_image(image, angle);
		if(rotated == NULL)
			return NULL;

		histogram = horizontal_histogram(rotated);
		if(histogram == NULL)
			return NULL;
		int val = max(histogram, image->height);
		if(val > max_val)
		{
			max_val = val;
			curr_angle = angle;
		}
		pgm_image_free(rotated);
		free(histogram);
	}
	rotated = rotate_image(image, curr_angle);
	if(rotated == NULL)
		return NULL;

	if(curr_angle > 0)
		*skew_angle = 360 - curr_angle;
	return rotated;
}
