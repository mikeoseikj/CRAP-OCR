#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "include/image.h"


int pgm_image_malloc(image_t **image)
{
	*image = malloc(sizeof(image_t));
	if((*image) == NULL)
		return -1;

	(*image)->pixels = NULL;
	return 0;
}

image_t *pgm_image_coord_malloc(int height, int width)
{
	image_t *image = malloc(sizeof(image_t));
	if(image == NULL)
		return NULL;

	image->height = height;
	image->width = width;

	image->pixels = malloc(sizeof(unsigned char *) * height); 
	if(image->pixels == NULL)
		return NULL;

	for(int i  = 0; i < height; i++)
	{
		image->pixels[i] = malloc(sizeof(unsigned char) * width);
		if(image->pixels[i] == NULL)
			return NULL;
	}

	return image;
}

image_t *pgm_image_dup(image_t *image)
{
	if(image == NULL)
		return NULL;
	image_t *out_image;
	if((out_image = pgm_image_coord_malloc(image->height, image->width)) == NULL)
		return NULL;

	out_image->height = image->height;
	out_image->width = image->width;
	out_image->max_color = image->max_color;

	for(int i = 0; i < image->height; i++)
	{
		for(int j = 0; j < image->width; j++)
			out_image->pixels[i][j] = image->pixels[i][j];
	}

	return out_image;
}

void pgm_image_free(image_t *image)
{
	if(image == NULL)
		return;

	for(int i = 0; i < image->height; i++)
	{
		if(image->pixels[i])
			free(image->pixels[i]);

	}
	if(image->pixels)
		free(image->pixels);
	free(image);
}


int pgm_image_read(char *filename, image_t **image)
{
	FILE *stream = fopen(filename, "rb");
	if(stream == NULL)
		return -1;

	struct stat statbuf;
	if(stat(filename, &statbuf) < 0)
		goto error;

	size_t filesize = statbuf.st_size;
	unsigned char *buf = malloc(filesize+1);
	if(buf == NULL)
		goto error;

	fscanf(stream, "%s", buf);
	if(ferror(stream))
		goto error;

	if(strcmp(buf, "P2") != 0 && strcmp(buf, "P5") != 0)	// Invalid image format
	{
		fclose(stream);
		return -2;
	}
	char *fmt = "%d";
	if(strcmp(buf, "P5") == 0)
		fmt = "%c";

	fscanf(stream, "%d%d%d", &((*image)->width), &((*image)->height), &((*image)->max_color));
	if(ferror(stream))
		goto error;
	int width = (*image)->width, height = (*image)->height;
	if(width == 0 || height == 0)
		goto error;
	(*image)->pixels = malloc(sizeof(unsigned char *) * height); 
	if((*image)->pixels == NULL)
		goto error;

	for(int i  = 0; i < height; i++)
	{
		(*image)->pixels[i] = malloc(sizeof(unsigned char) * width);
		if((*image)->pixels[i] == NULL)
			goto error;
	}

	unsigned char pixel;
	int count = 0, i = 0, j = 0;
	while(!feof(stream) && !ferror(stream))
	{
		fscanf(stream, fmt, &pixel);
		(*image)->pixels[i][j] = pixel;
		count++, j++;
		if(j == width)
			i++, j = 0;

		if(count == (width*height))
		{	
			free(buf);
			fclose(stream);
			return 0;
		}
	}
	error:
		if(buf)
			free(buf);
		fclose(stream);
		return -1;
}


int pgm_image_write(image_t *image, char *filename)
{
	FILE *stream = fopen(filename, "wb");
	if(stream == NULL)
		return -1;

	if(fwrite("P2\n", 1, 3, stream) != 3)
		goto error;

	char buf[32];
	sprintf(buf, "%d\t%d\n%d\n", image->width, image->height, image->max_color);
	int len = strlen(buf);
	if(fwrite(buf, 1, len, stream) != len)
		goto error;

	for(int i = 0; i < image->height; i++)
	{
		for(int j = 0; j < image->width; j++)
		{
			sprintf(buf, "%d\t", image->pixels[i][j]);
			len = strlen(buf);
			if(fwrite(buf, 1, len, stream) != len)
				goto error;
		}
		if(!fwrite("\n", 1, 1, stream))
			goto error;
	}

	fclose(stream);
	return 0;
	error:
		fclose(stream);
		return -1;
}