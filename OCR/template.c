#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "include/image.h"
#include "include/defines.h"
#include "include/template.h"
#include "include/utils.h"
#include "data/templates.dat"

int count_black_pixels(unsigned char tmplt[])
{
	int count = 0;
	for(int i = 0; i < TMPLT_ROWS; i++)
	{
		int k = i*TMPLT_COLS;
		for(int j = 0; j < TMPLT_COLS; j++)
		{
			if(tmplt[k+j] == 1)
				count++;
		}
	}
	return count;
}

// Euclidean distance is used but hamming distance can also be used
float distance(unsigned char *tmplt1, unsigned char *tmplt2, float ratio1, float ratio2)
{
	float features[7], sum = 0.0;
	for(int i = 0; i < TMPLT_SIZE; i++)
		sum += (float)((tmplt1[i]-tmplt2[i])*(tmplt1[i]-tmplt2[i]));            
	features[0] = sum;
	
	sum = 0.0;
	for(int i = 0; i < TMPLT_ROWS/2; i++)  // top left
	{
		int k = i*TMPLT_COLS;
		for(int j = 0; j < TMPLT_COLS/2; j++)
			sum += (float)((tmplt1[k+j]-tmplt2[k+j])*(tmplt1[k+j]-tmplt2[k+j]));
	}
	features[1] = sum;

	sum = 0.0;
	for(int i = TMPLT_ROWS/2; i < TMPLT_ROWS; i++)  // down left
	{
		int k = i*TMPLT_COLS;
		for(int j = 0; j < TMPLT_COLS/2; j++)
			sum += (float)((tmplt1[k+j]-tmplt2[k+j])*(tmplt1[k+j]-tmplt2[k+j]));
	}
	features[2] = sum;
	
	sum = 0.0;
	for(int i = 0; i < TMPLT_ROWS/2; i++)	// top right
	{
		int k = i*TMPLT_COLS;
		for(int j = TMPLT_COLS/2; j < TMPLT_COLS; j++)
			sum += (float)((tmplt1[k+j]-tmplt2[k+j])*(tmplt1[k+j]-tmplt2[k+j]));
	}
	
	features[3] = sum;
	sum = 0.0;
	for(int i = TMPLT_ROWS/2; i < TMPLT_ROWS; i++)	// down right
	{
		int k = i*TMPLT_COLS;
		for(int j = TMPLT_COLS/2; j < TMPLT_COLS; j++)
			sum += (float)((tmplt1[k+j]-tmplt2[k+j])*(tmplt1[k+j]-tmplt2[k+j]));
	}
	features[4] = sum;
	features[5] = (ratio1-ratio2)* (ratio1-ratio2);
	features[6] = (count_black_pixels(tmplt1)-count_black_pixels(tmplt2));

	sum = 0.0;
	for(int i  = 0; i < 7; i++)
		sum += (features[i]*features[i]);
	return sqrtf(sum);

}

char recognize_template(image_t *image, int *coords)
{
	unsigned char *input_tmplt = malloc(image->height*image->width);
	if(input_tmplt == NULL)
	{
		perror("recognize_template - error");
		exit(-1);
	}
	for(int i = 0; i < image->height; i++)
	{
		int k = i*image->width;
		for(int j = 0; j < image->width; j++)
			input_tmplt[k+j] = (image->pixels[i][j] == BLACK_PIXEL)? 1 : 0;
	}
	float ratio = ((float)(coords[DOWN_ROW]-coords[TOP_ROW])/(float)(coords[RIGHT_COL]-coords[LEFT_COL]));
	int match_index = 0;
	float dist, min_dist = 1000.0;
	for(int i = 0; i < NUM_TMPLT_CHARS; i++)
	{
		for(int j = 0; j < TMPLTS_PER_CHAR; j++)
		{
			dist = distance(input_tmplt, templates[i].tmplts[j].tmplt, ratio, templates[i].tmplts[j].ratio);
			if(dist < min_dist)
			{
				match_index = i;
				min_dist = dist;
			}
		}
	}

	free(input_tmplt);
	return templates[match_index].ascii_char;
};
