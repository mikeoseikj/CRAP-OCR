#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "include/image.h"
#include "include/segment.h"
#include "include/ocr.h"
#include "include/utils.h"
#include "include/nnet.h"
#include "include/defines.h"
#include "include/template.h"

glyph_t *glyph_queue = NULL;

void enqueue_glyph(glyph_t **queue, glyph_t *p)
{
	glyph_t *q = *queue;
	if(q == NULL)
	{
		*queue = p;
		p->next = NULL;
		return;
	}
	while(q->next)
		q = q->next;

	q->next = p;
	p->next = NULL;
}


void dequeue_glyph(glyph_t **queue, glyph_t *p)
{
    glyph_t *q = *queue;
    
    if(q == p)
    {
    	*queue = p->next;
    	return;
    }
    while(q && q->next != p)
    	q = q->next;
    q->next = p->next;
}

glyph_t *merge(glyph_t *list1, glyph_t *list2)
{
	if(list1 == NULL)
		return list2;
	if(list2 == NULL)
		return list1;

	if(list1->pos <= list2->pos) 
	{
		list1->next = merge(list1->next, list2);
		return list1;
	}
	else
	{
		list2->next = merge(list1, list2->next);
		return list2;
	}
}

glyph_t *split(glyph_t *list1)
{	
	if(list1 == NULL)
	{
		return NULL;
	}
	else if(list1->next == NULL)
	{
		return NULL;
	}
	else 
	{
		glyph_t *list2 = list1->next;
		list1->next = list2->next;
		list2->next = split(list2->next);
		return list2;
	}
}

glyph_t *rearrange_glyphs(glyph_t *list1)
{
	if(list1 == NULL)
	{
		return NULL;
	}
	else if(list1->next == NULL)
	{
		return list1;
	}
	else
	{
		glyph_t *list2 = split(list1);
		return merge(rearrange_glyphs(list1), rearrange_glyphs(list2));
	}
}

int rows_in_common(int a_top, int a_down, int b_top, int b_down)
{
	for(int i = a_top; i <= a_down; i++)
	{
		for(int j = b_top; j <= b_down; j++)
		{
			if(j == i)
				return 1;
		}
	}
	return 0;
}

int cols_in_common(glyph_t *glyph1, glyph_t *glyph2)
{
	for(int i = glyph1->coords[LEFT_COL]; i <= glyph1->coords[RIGHT_COL]; i++)
	{
		for(int j = glyph2->coords[LEFT_COL]; j <= glyph2->coords[RIGHT_COL]; j++)
		{
			if(j == i)
				return 1;
		}
	}
	return 0;
}

/*
	------------------------------------------ Separating Connected Characters/Glyphs ---------------------------------------------
	This split_glyph() function finds the first two potential split positions in an image with connected characters (moves 
	to next images if there are no potential split positions). 
	If there is only one split position(ie: no second split position), the end of the current main/parent image is taken as the 
	second split position. This two split position is used to create two sub images out the parent image and character recognitions 
	are done on each of them (ie: 2 separate recognitions). Also, a third recognition is done on an image produced by joining both 
	extracted/sub images. Then, the 'euclidean' distance of the third recognition is compared  with the average of the distances 
	of sub/extracted images (first two recognitions) and if third recognition has a smaller distance, the split is not taken 
	but if the average of the distances of the first two sub images is smaller, the split is taken. This process is repeated 
	on the parent image till no split positions are left.
	------------------------------------------------------------------------------------------------------------------------------- 
*/

int split_glyph(image_t *image, int *coords)
{
	int num_cols = (coords[RIGHT_COL]-coords[LEFT_COL])+1;
	int num_rows = (coords[DOWN_ROW]-coords[TOP_ROW])+1;
	float *histogram = malloc(sizeof(float)*num_cols);
	if(histogram == NULL)
		return 0;
	for(int i = 0; i < num_cols; i++)
		histogram[i] = 0.0;
	for(int i = coords[LEFT_COL]+1; i <= coords[RIGHT_COL]-1; i++)
	{
		for(int j = coords[TOP_ROW]; j <= coords[DOWN_ROW]; j++)
		{
			if(image->pixels[j][i] == BLACK_PIXEL)
				histogram[i-coords[LEFT_COL]]++;
		}
	}	

	int dosplit = 0, split_index;
	int split_pos1 = -1, split_pos2 = -1;
	image_t *first_seg, *second_seg = NULL, *both_seg;
	
	float denom = (num_cols < num_rows)? (float)num_cols : (float)num_rows;
	for(int i = 1; i < num_cols-1; i++)
	{
		if((histogram[i]/denom < 0.1) && (histogram[i-1]/histogram[i] >= 1.5 || histogram[i+1]/histogram[i] >= 1.5))
		{ 
			if(split_pos1 == -1)
			{
				split_pos1 = i;
			}
			else
			{
				split_pos2 = i;
				break;
			}
		}
	}
	
	if(split_pos1 == -1)
		return 0;

	// Extracting all the three segments
	if((first_seg = pgm_image_coord_malloc((coords[DOWN_ROW]-coords[TOP_ROW])+1, split_pos1)) == NULL)
	{
		perror("split - pgm_image_coord_malloc - error");
		exit(-1);
	}
	first_seg->max_color = image->max_color;
	for(int x = coords[TOP_ROW]; x <= coords[DOWN_ROW]; x++)
	{
		for(int y = coords[LEFT_COL]; y < coords[LEFT_COL]+split_pos1; y++)
			first_seg->pixels[x-coords[TOP_ROW]][y-coords[LEFT_COL]] = image->pixels[x][y];
	}


	if(split_pos2 > 0) 
	{
			if((both_seg = pgm_image_coord_malloc((coords[DOWN_ROW]-coords[TOP_ROW])+1, split_pos2)) == NULL)
			{
				perror("split - pgm_image_coord_malloc - error");
				exit(-1);
			}
			both_seg->max_color = image->max_color;
			for(int x = coords[TOP_ROW]; x <= coords[DOWN_ROW]; x++)
			{
				for(int y = coords[LEFT_COL]; y < coords[LEFT_COL]+split_pos2; y++)
					both_seg->pixels[x-coords[TOP_ROW]][y-coords[LEFT_COL]] = image->pixels[x][y];
			}
	}
	else
	{
		split_pos2 = num_cols;
		if((both_seg = pgm_image_coord_malloc((coords[DOWN_ROW]-coords[TOP_ROW])+1, split_pos2)) == NULL)
		{
			perror("split - pgm_image_coord_malloc - error");
			exit(-1);
		}
		both_seg->max_color = image->max_color;
		for(int x = coords[TOP_ROW]; x <= coords[DOWN_ROW]; x++)
		{
			for(int y = coords[LEFT_COL]; y <= coords[RIGHT_COL]; y++)
				both_seg->pixels[x-coords[TOP_ROW]][y-coords[LEFT_COL]] = image->pixels[x][y];
		}
	}

	if(split_pos1+1 < num_cols)
	{
		if((second_seg = pgm_image_coord_malloc((coords[DOWN_ROW]-coords[TOP_ROW])+1, split_pos2-split_pos1-1)) == NULL)
		{
			perror("split - pgm_image_coord_malloc - error");
			exit(-1);
		}
		second_seg->max_color = image->max_color;
		int y_off = coords[LEFT_COL]+split_pos1+1;
		for(int x = coords[TOP_ROW]; x <= coords[DOWN_ROW]; x++)
		{
			for(int y = y_off; y < coords[LEFT_COL]+split_pos2; y++)
				second_seg->pixels[x-coords[TOP_ROW]][y-y_off] = image->pixels[x][y];
		}
	}

	// For "first_seg"
	image_t *scaled_seg = resize_image(first_seg, TMPLT_ROWS, TMPLT_COLS);
	if(scaled_seg == NULL)
	{
		perror("split() - error");
		exit(-1);
	}
	unsigned char *input_tmplt = malloc(TMPLT_ROWS*TMPLT_COLS);
	if(input_tmplt == NULL)
	{
		perror("split - error");
		exit(-1);
	}
	
	for(int i = 0; i < TMPLT_ROWS; i++)
	{
		int k = i*TMPLT_COLS;
		for(int j = 0; j < TMPLT_COLS; j++)
			input_tmplt[k+j] = (scaled_seg->pixels[i][j] == BLACK_PIXEL)? 1 : 0;
	}

	
	float ratio = ((float)(coords[DOWN_ROW]-coords[TOP_ROW])/(float)split_pos1);
	float dist, min_dist = 1000;
	for(int i = 0; i < NUM_TMPLT_CHARS; i++)
	{
		for(int j = 0; j < TMPLTS_PER_CHAR; j++)
		{
			dist = distance(input_tmplt, templates[i].tmplts[j].tmplt, ratio, templates[i].tmplts[j].ratio);
			if(dist < min_dist)
				min_dist = dist;
		}
	}
	float first_seg_dist = min_dist;

	if(second_seg != NULL)
	{
		// For "second_seg"	
		free(scaled_seg);
		scaled_seg = resize_image(second_seg, TMPLT_ROWS, TMPLT_COLS);
		if(scaled_seg == NULL)
		{	
			perror("split() - error");
			exit(-1);
		}
		for(int i = 0; i < TMPLT_ROWS; i++)
		{
			int k = i*TMPLT_COLS;
			for(int j = 0; j < TMPLT_COLS; j++)
				input_tmplt[k+j] = (scaled_seg->pixels[i][j] == BLACK_PIXEL)? 1 : 0;
		}
		
		ratio = (float)(coords[DOWN_ROW]-coords[TOP_ROW])/(float)(split_pos2-split_pos1);	
		min_dist = 1000;
		for(int i = 0; i < NUM_TMPLT_CHARS; i++)
		{
			for(int j = 0; j < TMPLTS_PER_CHAR; j++)
			{
				dist = distance(input_tmplt, templates[i].tmplts[j].tmplt, ratio, templates[i].tmplts[j].ratio);
				if(dist < min_dist)
					min_dist = dist;
			}
		}
	}
	float second_seg_dist = min_dist;

	// For "both_seg"
	free(scaled_seg);
	scaled_seg = resize_image(both_seg, TMPLT_ROWS, TMPLT_COLS);
	if(scaled_seg == NULL)
	{
		perror("split() - error");
		exit(-1);
	}

	for(int i = 0; i < TMPLT_ROWS; i++)
	{
		int k = i*TMPLT_COLS;
		for(int j = 0; j < TMPLT_COLS; j++)
			input_tmplt[k+j] = (scaled_seg->pixels[i][j] == BLACK_PIXEL)? 1 : 0;
	}

	ratio = (float)(coords[DOWN_ROW]-coords[TOP_ROW])/(float)split_pos2;	
	min_dist = 1000;
	for(int i = 0; i < NUM_TMPLT_CHARS; i++)
	{
		for(int j = 0; j < TMPLTS_PER_CHAR; j++)
		{
			dist = distance(input_tmplt, templates[i].tmplts[j].tmplt, ratio, templates[i].tmplts[j].ratio);
			if(dist < min_dist)
				min_dist = dist;
		}
	}
	
	
	split_index = split_pos1;
	if(min_dist < ((first_seg_dist+second_seg_dist)/2))	// min_dist can be referred to as "both_seg_dist"
		split_index = split_pos2;
	else 
		dosplit = 1;

	if(dosplit)
	{
		for(int i = coords[LEFT_COL]; i <= coords[RIGHT_COL]; i++)
		{
			if(i == (split_index+coords[LEFT_COL]))
			{
				for(int j = coords[TOP_ROW]; j <= coords[DOWN_ROW]; j++)
					image->pixels[j][i] = WHITE_PIXEL;
			}
		}
	}

	pgm_image_free(first_seg);
	pgm_image_free(second_seg);
	pgm_image_free(both_seg);
	pgm_image_free(scaled_seg);
	free(histogram);
	return dosplit;
}

/*
	The extraction here is done by finding farmost/edge pixels of enclosed black regions/characters by using a 
	flood fill algorithm. This approach is efficient and accurate but it has a serious disadvantage ie. if white 
	spots(lines/pixels) can be traced across the character(the unextracted character in the document) then those
	separated part(s) in the character in question won't be treated as part(s) of the character rather they will
	be treated as separate individual character(s) during extraction.

*/

glyph_t *extract_all_glyphs(image_t *image, int *avg_width)
{
	image_t *tmp_image = pgm_image_dup(image);
	if(tmp_image == NULL)
		return NULL;

	char buf[32];
	*avg_width = 0;
	image_t *bbox_image;
	glyph_t *glyph;
	int coords[4], line = 0, count = 0, spacing = 0;
	int lmin = -1, lmax = -1;
	for(int i = 0; i < tmp_image->height; i++)
	{
		for(int j = 0; j < tmp_image->width; j++)
		{
			if(tmp_image->pixels[i][j] == BLACK_PIXEL)
			{
				coords[TOP_ROW] = INT_MAX, coords[DOWN_ROW] = -1, coords[LEFT_COL] = INT_MAX, coords[RIGHT_COL] = -1;
				image_t *tmp = pgm_image_dup(tmp_image);
				if(tmp == NULL)
					return NULL;
				get_char_enclose_box(tmp_image, i, j, &coords[0]);
				if(split_glyph(tmp, coords))
				{
					j--;
					pgm_image_free(tmp_image);
					tmp_image = tmp;
					continue;

				}
				pgm_image_free(tmp);
				if((bbox_image = pgm_image_coord_malloc((coords[DOWN_ROW]-coords[TOP_ROW])+1, (coords[RIGHT_COL]-coords[LEFT_COL])+1)) == NULL)
					return NULL;
				bbox_image->max_color = tmp_image->max_color;
				for(int i = coords[TOP_ROW]; i <= coords[DOWN_ROW]; i++)
				{
					for(int j = coords[LEFT_COL]; j <= coords[RIGHT_COL]; j++)
						bbox_image->pixels[i-coords[TOP_ROW]][j-coords[LEFT_COL]] = image->pixels[i][j];
				}
				
				glyph = malloc(sizeof(glyph_t));
				if(glyph == NULL)
					return NULL;
				glyph->next = NULL;
				glyph->image = resize_image(bbox_image, IMAGE_HEIGHT, IMAGE_WIDTH);
				pgm_image_free(bbox_image);
				*avg_width += (coords[RIGHT_COL]-coords[LEFT_COL]);
				count++;
				memcpy(&glyph->coords[0], &coords[0], sizeof(coords));
				enqueue_glyph(&glyph_queue, glyph);

				if(lmin < 0)
					lmin = glyph->coords[TOP_ROW], lmax = glyph->coords[DOWN_ROW];

				if(!rows_in_common(glyph->coords[TOP_ROW], glyph->coords[DOWN_ROW], lmin, lmax))
				{
					lmin = glyph->coords[TOP_ROW], lmax = glyph->coords[DOWN_ROW]; 
					line++;
				}
				else
				{
					if(lmin > glyph->coords[TOP_ROW])
						lmin = glyph->coords[TOP_ROW];
					if(lmax < glyph->coords[DOWN_ROW])
						lmax = glyph->coords[DOWN_ROW];
				}

				glyph->line = line;
				glyph->pos = (line*tmp_image->width) + coords[LEFT_COL];
			}
		} 
	}

	if(count > 0)
	{
		*avg_width = *avg_width/count;
		glyph = glyph_queue;
		while(glyph)	// removes some noise
		{			
			if((glyph->coords[RIGHT_COL]-glyph->coords[LEFT_COL]) < ((*avg_width)/5))
			{
				dequeue_glyph(&glyph_queue, glyph);
				glyph_t *tmp = glyph;
				glyph = glyph->next;
				free(tmp);
				continue;
			}
			glyph = glyph->next;
		}
	}

	glyph_queue = rearrange_glyphs(glyph_queue);
	free(tmp_image);
	return glyph_queue;
}



char *template_recognition(glyph_t *extracts, int average_width)
{
	glyph_t *glyph = extracts;
	glyph_t *topmost, *downmost;
	int topmost_height, downmost_height, topmost_width, downmost_width;
	int space, size = 0;
	char character, *output = NULL;
	
	while(glyph)
	{
		character = recognize_template(glyph->image, glyph->coords);	
		if(glyph->next && glyph->line == glyph->next->line && cols_in_common(glyph, glyph->next))
		{
			if((glyph->coords[TOP_ROW] <= glyph->next->coords[TOP_ROW]))	
			{
				topmost = glyph;
				downmost = glyph->next;
			}
			else
			{
				topmost = glyph->next;
				downmost = glyph;
			}

			downmost_height = downmost->coords[DOWN_ROW]-downmost->coords[TOP_ROW];
			topmost_height = topmost->coords[DOWN_ROW]-topmost->coords[TOP_ROW];
			topmost_width = topmost->coords[RIGHT_COL]-topmost->coords[LEFT_COL];
			downmost_width = downmost->coords[RIGHT_COL]-downmost->coords[LEFT_COL];

			if(downmost_height >= topmost_height)
			{
				if(topmost_height == 0)
				{
					glyph = glyph->next;
					continue;
				}
				float height_factor = (float)downmost_height/(float)topmost_height;
				if(character == 'j')	// j is 'unique' and could've been correctly been identified
					character = 'j';
				else if(height_factor <= 1.5 && ((float)topmost_width > ((float)average_width*0.5)))
					character = '=';
				else if(height_factor <= 1.5)	// this condition will also execute if heights are equal
					character = ':';
				else if(height_factor <= 3.0)	
					character = ';';
				else if(height_factor <= 18.0)
					character = 'i';
				else
					character = 'j';

				glyph = glyph->next; // skip next
				int down_right = downmost->coords[RIGHT_COL], top_right = topmost->coords[RIGHT_COL];
				int rightmost = (down_right > top_right)? down_right:top_right;
				if(glyph->next)
					space = glyph->next->coords[LEFT_COL]-rightmost;
			}
			else
			{
				if(downmost_width == 0)
				{
					glyph = glyph->next;
					continue;
				}
				float width_factor = (float)topmost_width/(float)downmost_width;
				float height_factor = (float)topmost_height/(float)downmost_height;
				if(width_factor >= 2.0) // width of "hook" for '?' twice or more times bigger than "dot"
					character = '?';
				else if(height_factor <= 1.5 && ((float)topmost_width > ((float)average_width*0.5)))
					character = '=';
				else if(height_factor <= 1.5)	
					character = ':';
				else 
					character = '!';

				glyph = glyph->next; // skip next
				int down_right = downmost->coords[RIGHT_COL], top_right = topmost->coords[RIGHT_COL];
				int rightmost = (down_right > top_right)? down_right:top_right;
				if(glyph->next)
					space = glyph->next->coords[LEFT_COL]-rightmost;
			}
				
		}
		else 
		{
			if(glyph->next)
				space = glyph->next->coords[LEFT_COL]-glyph->coords[RIGHT_COL];
		}
		
		if(space < 0)
			space = 0;
		int num_spaces = space/average_width;
		size += (1+num_spaces);
		if((output = realloc(output, size)) == NULL)
			goto realloc_error;

		output[(size-num_spaces)-1] = character;
		for(int i = size-1; i >= (size-num_spaces); i--)
			output[i] = ' ';
		
		if(glyph->next && glyph->line != glyph->next->line) // newline
		{
			size++;
			if((output = realloc(output, size)) == NULL)
				goto realloc_error;
			output[size-1] = '\n';
		}
		if(glyph)
			glyph = glyph->next;
	}

	if((output = realloc(output, ++size)) == NULL) // for '\0'
		goto realloc_error;
	output[size-1] = '\0';

	return output;
	realloc_error:
		perror("error - template_recognition");
		return NULL;
}



static int max(float *buf, int len)
{
	int index = 0;
	float val = buf[0];
	for(int i = 1; i < len; i++)
	{
		if(buf[i] > val)
		{
			val = buf[i];
			index = i;
		}
	}
	return index;
}


char *neural_network_recognition(glyph_t *extracts, int average_width)
{
	glyph_t *glyph = extracts;
	glyph_t *topmost, *downmost;
	float *nn_input = alloc_float_array(NN_INPUT_NODES);
	if(nn_input == NULL)
		return NULL;

	nnet_t *network = fbp_create_network(NN_INPUT_NODES, NN_HIDDEN_NODES, NN_OUTPUT_NODES);
	if(network == NULL)
	{
		perror("error - fbp_create_network");
		return NULL;
	}
	if(fbp_load_weights(network, "data/weights.dat") < 0)
	{
		perror("error - fbp_load_weights");
		return NULL;
	}

	int space, index, size = 0;
	int topmost_height, downmost_height, topmost_width, downmost_width;
	char character, *output = NULL;

	while(glyph)
	{
		for(int i = 0; i < network->num_output_nodes; i++)
			network->output_layer.outputs[i] = 0.0;
		image_to_nn_input(nn_input, glyph->image);
		fbp_set_run_inputs(network, nn_input);
		fbp_forward_propagate(network);
       
		index = max(network->output_layer.outputs, network->num_output_nodes);
		character = mapping[index];
		
		if(glyph->next && glyph->line == glyph->next->line && cols_in_common(glyph, glyph->next))
		{
			if((glyph->coords[TOP_ROW] <= glyph->next->coords[TOP_ROW]))	
			{
				topmost = glyph;
				downmost = glyph->next;
			}
			else
			{
				topmost = glyph->next;
				downmost = glyph;
			}

			downmost_height = downmost->coords[DOWN_ROW]-downmost->coords[TOP_ROW];
			topmost_height = topmost->coords[DOWN_ROW]-topmost->coords[TOP_ROW];
			topmost_width = topmost->coords[RIGHT_COL]-topmost->coords[LEFT_COL];
			downmost_width = downmost->coords[RIGHT_COL]-downmost->coords[LEFT_COL];

			if(downmost_height >= topmost_height)
			{
				if(topmost_height == 0)
				{
					glyph = glyph->next;
					continue;
				}
				float height_factor = (float)downmost_height/(float)topmost_height;
				if(mapping[index] == 'j')	// j is 'unique' and could've been correctly been identified
					character = 'j';	
				else if(height_factor <= 1.5 && ((float)topmost_width > ((float)average_width*0.5)))
					character = '=';
				else if(height_factor <= 1.5)	// this condition will also execute if heights are equal
					character = ':';
				else if(height_factor <= 3.0)	
					character = ';';
				else if(height_factor <= 18.0)
					character = 'i';
				else
					character = 'j';

				glyph = glyph->next; // skip next
				int down_right = downmost->coords[RIGHT_COL], top_right = topmost->coords[RIGHT_COL];
				int rightmost = (down_right > top_right)? down_right:top_right;
				if(glyph->next)
					space = glyph->next->coords[LEFT_COL]-rightmost;
			}
			else
			{
				if(downmost_width == 0)
				{
					glyph = glyph->next;
					continue;
				}
				float width_factor = (float)topmost_width/(float)downmost_width;
				float height_factor = (float)topmost_height/(float)downmost_height;
				if(width_factor >= 2.0) // width of "hook" for '?' twice or more times bigger than "dot"
					character = '?';
				else if(height_factor <= 1.5 && ((float)topmost_width > ((float)average_width*0.5)))
					character = '=';
				else if(height_factor <= 1.5)	
					character = ':';
				else 
					character = '!';

				glyph = glyph->next; // skip next
				int down_right = downmost->coords[RIGHT_COL], top_right = topmost->coords[RIGHT_COL];
				int rightmost = (down_right > top_right)? down_right:top_right;
				if(glyph->next)
					space = glyph->next->coords[LEFT_COL]-rightmost;
			}
				
		}
		else 
		{
			if(glyph->next)
				space = glyph->next->coords[LEFT_COL]-glyph->coords[RIGHT_COL];
		}
			
		
		if(space < 0)
			space = 0;
		int num_spaces = space/average_width;
		size += (1+num_spaces);
		if((output = realloc(output, size)) == NULL)
			goto realloc_error;

		output[(size-num_spaces)-1] = character;
		for(int i = size-1; i >= (size-num_spaces); i--)
			output[i] = ' ';
		
		if(glyph->next && glyph->line != glyph->next->line) // newline
		{
			size++;
			if((output = realloc(output, size)) == NULL)
				goto realloc_error;
			output[size-1] = '\n';
		}
		if(glyph)
			glyph = glyph->next;
	}

	if((output = realloc(output, ++size)) == NULL) // for '\0'
		goto realloc_error;
	output[size-1] = '\0';

	free(nn_input);
	return output;

	realloc_error:
		free(nn_input);
		perror("error - neural_network_recognition");
		return NULL;
}



float spell_distance(char *str1, char *str2)
{
	//printf("str1 = %s   str2 = %s\n", str1, str2);
	int max = (strlen(str1) > strlen(str2))? strlen(str1): strlen(str2);
	int min = (strlen(str1) < strlen(str2))? strlen(str1): strlen(str2);
	float dist = 0.0;
	float denom = MAX_WORD_LEN*max;
	for(int i = 0; i < min; i++)
	{
		if(str1[i] == str2[i])
			dist += (MAX_WORD_LEN-i);
	}
	for(int i = 0; i < max; i++)
		denom -= i;
	dist = dist/denom;
	return dist;
}

int is_split_char(char *str)
{
	char *ptr = str;
	if(*ptr == ' ' || *ptr == '\n' || *ptr == '\0')
		return 1;
	if(ispunct(*ptr) && *ptr != '(' && *ptr != '{' && *ptr != '[')
		return 1;
	if(!ispunct(*ptr))
		return 0;

	// checking if brackets are closed
	ptr++;
	while(*ptr != ' ' && *ptr != '\0' && *ptr != '\n')
	{
		if(*ptr == '(' || *ptr == '{' || *ptr == '[')
			return 0;
		if(*ptr == ')' || *ptr == '}' || *ptr == ']')
			return 1;
		ptr++;
	}
	return 0;
}

