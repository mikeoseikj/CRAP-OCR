#include <string.h>
#include <math.h>

#include "include/image.h"

void threshold_image(image_t *image)
{
	int threshold, size = 256;
	int histogram[size];
	memset(histogram, 0, size*sizeof(int));

	for(int i = 0; i < image->height; i++)
	{
		for(int j = 0; j < image->width; j++)
			histogram[(int)(image->pixels[i][j])]++;
	}

	int n_pixels = image->height*image->width;
	float tot_sum = 0;
	for(int i = 0; i < size; i++)
		tot_sum += i*histogram[i];

	float q1 = 0, q2 = 0, u1 = 0, u2 = 0; 
	float max_variance = 0, variance, sum = 0;
	for(int i = 0; i < size; i++)
	{
		q1 += histogram[i];
		if(q1 == 0)
			continue;
		q2 = n_pixels-q1;
		if(q2 == 0)
			break;

		sum += (i*histogram[i]);
		u1 = sum/q1;
		u2 = (tot_sum-sum)/q2;

		variance = q1*q2*((u1-u2)*(u1-u2));
		if(variance > max_variance)
		{
			threshold = i;
			max_variance = variance;
		}
	}

	for(int i = 0; i < image->height; i++)
	{
		for(int j = 0; j < image->width; j++)
		{
			if((int)image->pixels[i][j] >= threshold)
				image->pixels[i][j] = WHITE_PIXEL;
			else
				image->pixels[i][j] = BLACK_PIXEL;
		}
	}
}


image_t *resize_image(image_t *image, int new_height, int new_width)
{

	image_t *output_image = pgm_image_coord_malloc(new_height, new_width);
	if(output_image == NULL)
		return NULL;

	output_image->max_color = image->max_color;
	float h_rate = (float)new_height/(float)image->height;
	float w_rate = (float)new_width/(float)image->width;

	int x, y;
	for(int i = 0; i < new_height; i++)
	{
		x = (int)floor((float)i/h_rate);
		for(int j = 0; j < new_width; j++)
		{
			y = (int)floor((float)j/w_rate);
			output_image->pixels[i][j] = image->pixels[x][y];
		}
	}

	return output_image;
}
