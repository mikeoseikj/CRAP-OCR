#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "include/utils.h"
#include "include/dict_words.h"

float *alloc_float_array(int size)
{
	float *addr = malloc(sizeof(float)*size);
	if(addr == NULL)
	{
		perror("error - alloc_double_array");
		return NULL;
	}
	return addr;
}

void get_char_enclose_box(image_t *image, int i, int j, int *coords)
{
	if(i < 0 || i >= image->height)
		return;
	if(j < 0 || j >= image->width)
		return;
	if(image->pixels[i][j] == WHITE_PIXEL)
		return;

	image->pixels[i][j] = WHITE_PIXEL;
	if(i < coords[TOP_ROW])  
		coords[TOP_ROW] = i;
	if(i > coords[DOWN_ROW]) 
		coords[DOWN_ROW] = i;

	if(j < coords[LEFT_COL]) 
		coords[LEFT_COL] = j;
	if(j > coords[RIGHT_COL]) 
		coords[RIGHT_COL] = j;

	get_char_enclose_box(image, i-1, j, coords);
	get_char_enclose_box(image, i+1, j, coords);
	get_char_enclose_box(image, i, j-1, coords);
	get_char_enclose_box(image, i, j+1, coords);
}

void image_to_nn_input(float *output, image_t *image)
{
	for(int i = 0; i < image->height; i++)
	{
		int k = i*image->width;
		for(int j = 0; j < image->width; j++)
			output[k+j] = (image->pixels[i][j] == BLACK_PIXEL)? 1.0 : -1;
	}
}



void strtolower(char *str)
{
	while(*str)
	{
		if(isupper(*str))
			*str = tolower(*str);
		str++;
	}
}

int digits_in_word(char *str)
{
	int alpha = 0, digit = 0;
	while(*str)
	{
		if(isalpha(*str))
			alpha = 1;
		else if(isdigit(*str))
			digit = 1;
		str++;
	}
	if(alpha && digit)
		return 1;
	return 0;
}

static void match_word(char *word, struct dword dict_word, int start, float *max_score, float *max_prob, char *corrected, int mode)
{
	int prev_match_index = 0, count = 0;
	float score = 0.0;
	int word_len = strlen(word), len = strlen(dict_word.str);
	if(start >= len)
		return;
	
	for(int i = start; i < len; i++)
	{
		for(int k = count; k < word_len; k++)
		{
			if(word[k] == dict_word.str[i])
			{
				count = k+1;
				score += (MAX_WORD_LEN - (i-prev_match_index) - k);
				prev_match_index = i;		
				break;
			}	
		}
	}
	
	char *tmp = strdup(word);
	if(tmp == NULL)
	{
		perror("spell_check - strdup");
		exit(-1);
	}
	for(int i = 0; i < len; i++)
	{
		for(int j = 0; j < word_len; j++)
		{
			if(dict_word.str[i] == tmp[j])
			{
				tmp[j] = '\0';
				score += (MAX_WORD_LEN - ((i-j)*(i-j)));
				break;
			}
		}
	}
	free(tmp);
	
	if(word_len == len && mode == REGARD_LENGTH)	// Remove this statement if this function is not going to be used in this program
		score += MAX_WORD_LEN;
	
	if(len > word_len)
		score = score*((float)word_len/(float)len); 
	if(score > *max_score)
	{
		*max_score = score;
		*max_prob = dict_word.prob;
		strcpy(corrected, dict_word.str);	
	}
	if(score == *max_score && dict_word.prob > *max_prob)
	{
		*max_prob = dict_word.prob;
		strcpy(corrected, dict_word.str);	
	}
	match_word(word, dict_word, start+1, max_score, max_prob, corrected, mode);
}


char *spell_check(char *word, int mode)
{
	int index = 0;
	float max_score = -1.0, max_prob = -1.0;
	struct dword dict_word = DICT_WORDS[index];
	char *corrected = malloc(MAX_WORD_LEN+1);
	if(corrected == NULL)
	{
		perror("error - spell_check");
		exit(-1);
	}
	while(index < MAX_DICT_WORDS)
	{
		if(strlen(dict_word.str) >= (2*strlen(word)))
		{
			dict_word = DICT_WORDS[++index];
			continue;
		}
		match_word(word, dict_word, 0, &max_score, &max_prob, corrected, mode);
		dict_word = DICT_WORDS[++index];
	}	
	return corrected;	
}

