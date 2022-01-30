#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "include/image.h"
#include "include/segment.h"
#include "include/ocr.h"
#include "include/skew.h"
#include "include/utils.h"

int main(int argc, char *argv[])
{
	if(argc < 2)	
	{
		fprintf(stderr, "%s <PGM image file>\n", argv[0]);
		return -1;
	}

	char *imgfile = argv[1];
	int err, angle;
	image_t *image;

	if(pgm_image_malloc(&image) < 0)
	{
		perror("error - pgm_image_malloc");
		return -1;
	}
	if((err = pgm_image_read(imgfile, &image)) < 0)
	{
		if(err == -1)
			perror("error - pgm_image_read");
		else
			printf("error: Invalid PGM file format\n");
		return -1;
	}
		
	threshold_image(image);
	image_t *deskewed_image = deskew(image, &angle);
	if(deskewed_image == NULL)
	{
		perror("deskewed_image");
		return -1;
	}	
	pgm_image_free(image);

	int average_width;
	glyph_t *extracts = extract_all_glyphs(deskewed_image, &average_width);
	glyph_t *g, *glyph = extracts;
	if(extracts == NULL)
	{
		perror("error - extract_all_glyphs");
		return -1;
	}

	int count = 0;
	char *output1 = template_recognition(extracts, average_width);
	char *output2 = neural_network_recognition(extracts, average_width);

	char *tmplt_word, *nnet_word, *mem1, *mem2;
	tmplt_word = mem1= strdup(output1);
	nnet_word = mem2 = strdup(output2);
	if(tmplt_word == NULL || nnet_word == NULL)
	{
		perror("strdup");
		return -1;
	}
	
	char *corrected, *output = NULL, term_byte;
	int len, offset, has_space, size = 0;
	float dist1, dist2;

	for(int i = 0; i < strlen(tmplt_word)+1; i++)
	{
		if(is_split_char(&tmplt_word[i]))
		{
			term_byte = tmplt_word[i];
			tmplt_word[i] = nnet_word[i] = '\0';
			len = strlen(tmplt_word);
			if(len == 0)
			{
				if((output = realloc(output, ++size)) == NULL)
				{
					perror("error");
					return -1;
				}
				output[size-1] = term_byte;
				goto END;
			}
		
			strtolower(tmplt_word);
			strtolower(nnet_word);
			int exact_match = 0;
			corrected = spell_check(tmplt_word, REGARD_LENGTH);	
			if(strcmp(corrected, tmplt_word) == 0)	// check if template matching's recognized string is a dictionary words
			{
				dist1 = spell_distance(corrected, tmplt_word);
				exact_match++;
			}
				
			if(!exact_match)
			{
				free(corrected);
				corrected = spell_check(nnet_word, REGARD_LENGTH);	// check if neural network's recognized string is a dictionary words
				if(strcmp(corrected, nnet_word) == 0)
					exact_match++;
			}
			if(!exact_match)
			{
				// Compare 'tmplt_word' and 'nnet_word' directly
				free(corrected);
				dist1 = spell_distance(tmplt_word, nnet_word);
				corrected = strdup(tmplt_word);
			
				// Spell check 'tmplt_word' and compare with itself
				char *word = spell_check(tmplt_word, REGARD_LENGTH);
				dist2 = spell_distance(word, tmplt_word);
				if(dist2 > dist1)
				{
					dist1 = dist2;
					free(corrected);
					corrected = word;
				}
				else
				{
					free(word);
				}
				
				// Replacing unmatched 'tmplt_word' characters with that of 'nnet_word' characters
				char *tmp = malloc(len+1);
				tmp[len] = '\0';
				for(int i = 0; i < len; i++)
				{
					if(tmplt_word[i] != nnet_word[i])	
						tmp[i] = nnet_word[i];	
					else
						tmp[i] = tmplt_word[i];
				}

				// Spell check substituted string and compare with 'tmplt_word'
				word = spell_check(tmp, REGARD_LENGTH);
				dist2 = spell_distance(word, tmplt_word);
				if(dist2 > dist1)
				{
					dist1 = dist2;
					free(corrected);
					corrected = word;
				}
				else
				{
					free(word);
				}
		
				// Assuming some digit and symbols are misrecognized letters and subsituting them
				for(int i = 0; i < len; i++)	
				{
					if(tmplt_word[i] == '0')
						tmp[i] = 'o';	
					else if(tmplt_word[i] == '1')
						tmp[i] = 'l';
					else if(tmplt_word[i] == '7')
						tmp[i] = 'T';
					else if(tmplt_word[i] == '8')
							tmp[i] = 'B';
					else if(tmplt_word[i] == '9')
						tmp[i] = 'g';
					else if(tmplt_word[i] == '[' || tmplt_word[i] == '{')
						tmp[i] = 'r';
					else if(tmplt_word[i] == '(')
						tmp[i] = 'c';
					else
						tmp[i] = tmplt_word[i];
				}
				
				// Spell check substituted string and compare with itself	
				word = spell_check(tmp, REGARD_LENGTH);
				dist2 = spell_distance(word, tmp);	
				if(dist2 > dist1)	
				{
					free(corrected);
					corrected = word;
				}
				else
				{
					free(word);
				}	
				free(tmp);
				
			}

			size += (strlen(corrected)+1); // +"term_byte"
			if((output = realloc(output, size)) == NULL)
			{
				perror("error");
				return -1;
			}
			offset = 0;
			for(int i = (size-strlen(corrected))-1; i < size-1; i++)
				output[i] = corrected[offset++];
			
			output[size-1] = term_byte;
			
			// For a case where spell corrected word is longer than input
			for(int k = 0; k < strlen(corrected)-len; k++)
				i++;

			free(corrected);
			END:
				if(term_byte != '\0')
				{
					tmplt_word = &tmplt_word[i+1];
					nnet_word = &nnet_word[i+1];
					i = -1;
				}
				
		}	
	}	

	
	output[size-1] = '\0';
	// Recapilize first letter of words that were initialy recognized as capital letters
	len = (strlen(output) < strlen(output1))? strlen(output) : strlen(output1); 
	for(int i = 0; i < len; i++)
	{
		if(((toupper(output[i]) == output1[i]) || (toupper(output[i]) == output2[i])) && (i == 0 || output[i-1] == ' '))
			output[i] = toupper(output[i]);
	}

	// Display necessary info and recognized text
	printf("OCR_HEADER:");	// Needed by the webapp for processing
	printf("Image Width: %d pixels<br />", deskewed_image->width);
	printf("Image Height: %d pixels<br />", deskewed_image->height);
	printf("Skewness: ");
	if(angle == 0)
		printf("Image is not skewed");
	else if(angle == 1)
		printf("Image is skewed at %d degree clockwise", angle);
	else
		printf("Image is skewed at %d degrees clockwise", angle);
	printf("OCR_CONTENT:");	// Needed by the webapp for processing
	printf("%s\n", output);

	free(output1);
	free(output2);
	free(mem1);
	free(mem2);
	pgm_image_free(deskewed_image);
	return 0;
}
