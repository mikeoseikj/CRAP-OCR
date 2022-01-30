#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "include/nnet.h"
#include "include/utils.h"
#include "include/defines.h"

static float sigmoid(float value)
{
	return (float)(1.0/(1.0+exp(-(double)value)));
}

nnet_t *fbp_create_network(int num_input_nodes, int num_hidden_nodes, int num_output_nodes)
{
	nnet_t *network = malloc(sizeof(nnet_t));
	if(network == NULL)
		return NULL;

	network->num_input_nodes = num_input_nodes;
	network->num_hidden_nodes = num_hidden_nodes;
	network->num_output_nodes = num_output_nodes;

	network->input_layer.inputs  = alloc_float_array(num_input_nodes);
	network->input_layer.outputs = alloc_float_array(num_input_nodes);
	if(network->input_layer.inputs == NULL || network->input_layer.outputs == NULL)
		return NULL;
	
	network->hidden_layer.inputs  = alloc_float_array(num_input_nodes);
	network->hidden_layer.outputs = alloc_float_array(num_hidden_nodes);
	network->hidden_layer.weights = alloc_float_array(num_input_nodes*num_hidden_nodes);
	network->hidden_layer.errors  = alloc_float_array(num_hidden_nodes);
	network->hidden_layer.prev_weights = alloc_float_array(num_input_nodes*num_hidden_nodes);
	if(network->hidden_layer.inputs == NULL || network->hidden_layer.outputs == NULL || network->hidden_layer.weights == NULL)
		return NULL;
	if(network->hidden_layer.errors == NULL || network->hidden_layer.prev_weights == NULL)
		return NULL;

	network->output_layer.inputs  = alloc_float_array(num_hidden_nodes);
	network->output_layer.outputs = alloc_float_array(num_output_nodes);
	network->output_layer.weights = alloc_float_array(num_hidden_nodes*num_output_nodes);
	network->output_layer.errors  = alloc_float_array(num_output_nodes);
	network->output_layer.expected  = alloc_float_array(num_output_nodes);
	network->output_layer.prev_weights = alloc_float_array(num_hidden_nodes*num_output_nodes);
	if(network->output_layer.inputs == NULL || network->output_layer.outputs == NULL || network->output_layer.weights == NULL)
		return NULL;
	if(network->output_layer.errors == NULL || network->output_layer.expected == NULL || network->output_layer.prev_weights == NULL)
		return NULL;

	return network;
}

void fbp_set_train_inputs(nnet_t *network, float input[], float expected[])
{
	for(int i = 0; i < network->num_input_nodes; i++)
		network->input_layer.inputs[i] = network->input_layer.outputs[i] = input[i];
	for(int i = 0; i < network->num_output_nodes; i++)
		network->output_layer.expected[i] = expected[i];
}

void fbp_set_run_inputs(nnet_t *network, float input[])
{
	for(int i = 0; i < network->num_input_nodes; i++)
		network->input_layer.inputs[i] = network->input_layer.outputs[i] = input[i];
}

void fbp_set_random_weights(nnet_t *network)
{
	float rnum, min = -0.5, max = 0.5;
	srand(time(NULL));
	for(int i = 0; i < network->num_input_nodes*network->num_hidden_nodes; i++)
	{
		rnum = ((max-min)*((float)rand()/RAND_MAX))+min;
		network->hidden_layer.weights[i] = rnum;
		network->hidden_layer.prev_weights[i] = 0.0;
	}
	for(int i = 0; i < network->num_hidden_nodes*network->num_output_nodes; i++)
	{
		rnum = ((max-min)*((float)rand()/RAND_MAX))+min;
		network->output_layer.weights[i] = rnum;
		network->output_layer.prev_weights[i] = 0.0;
	}
}

void fbp_forward_propagate(nnet_t *network)
{
	float sump;
	for(int i = 0; i < network->num_input_nodes; i++)
		network->hidden_layer.inputs[i] = network->input_layer.outputs[i];
	for(int i = 0; i < network->num_hidden_nodes; i++)
	{
		sump = 0.0;
		for(int j = 0; j < network->num_input_nodes; j++)
		{
			int k = j*network->num_hidden_nodes;
			sump += network->hidden_layer.inputs[j]*network->hidden_layer.weights[k+i];
		}
		network->hidden_layer.outputs[i] = sigmoid(sump);
	}
	

	for(int i = 0; i < network->num_hidden_nodes; i++)
		network->output_layer.inputs[i] = network->hidden_layer.outputs[i];
	for(int i = 0; i < network->num_output_nodes; i++)
	{
		sump = 0.0;
		for(int j = 0; j < network->num_hidden_nodes; j++)
		{
			int k = j*network->num_output_nodes;
			sump += network->output_layer.inputs[j]*network->output_layer.weights[k+i];
		}
		network->output_layer.outputs[i] = sigmoid(sump);
	}
}

void fbp_backward_propagate(nnet_t *network, float *error)
{
	*error = 0.0;
	for(int i = 0; i < network->num_output_nodes; i++)
	{
		network->output_layer.errors[i] = network->output_layer.expected[i]-network->output_layer.outputs[i];
		*error += network->output_layer.errors[i];
	}

	for(int i = 0; i < network->num_hidden_nodes; i++)
	{
		float sump = 0.0;
		int k = i*network->num_output_nodes;
		for(int j = 0; j < network->num_output_nodes; j++)
			sump += (network->output_layer.weights[k+j]*network->output_layer.errors[j]); 
		network->hidden_layer.errors[i] = sump * (network->hidden_layer.outputs[i] * (1.0-network->hidden_layer.outputs[i]));
	}
}

void fbp_update_weights(nnet_t *network)
{
	float weight;
	for(int i = 0; i < network->num_input_nodes; i++)
	{
		int k = i*network->num_hidden_nodes;
		for(int j = 0; j < network->num_hidden_nodes; j++)
		{
			weight = (network->learn_rate*network->hidden_layer.errors[j]*network->hidden_layer.inputs[i]) + (network->momentum*network->hidden_layer.prev_weights[k+j]);
			network->hidden_layer.prev_weights[k+j] = weight;
			network->hidden_layer.weights[k+j] += weight;
		}
	}

	for(int i = 0; i < network->num_hidden_nodes; i++)
	{
		int k =  i*network->num_output_nodes;
		for(int j = 0; j < network->num_output_nodes; j++)
		{
			weight = (network->learn_rate*network->output_layer.errors[j]*network->output_layer.inputs[i]) + (network->momentum*network->output_layer.prev_weights[k+j]);
			network->output_layer.prev_weights[i] = weight;
			network->output_layer.weights[k+j] += weight;
		}
	}
}


int fbp_store_weights(nnet_t *network, char *datafile)
{	
	FILE *stream = fopen(datafile, "wb");
	if(stream == NULL)
		return -1;

	for(int i = 0; i < network->num_input_nodes; i++)
	{
		int k = i*network->num_hidden_nodes;
		for(int j = 0; j < network->num_hidden_nodes; j++)
		{
			if(fprintf(stream, "%f\t", network->hidden_layer.weights[k+j]) < 0)
				goto error;
		}
		if(fprintf(stream, "\n") < 0)
			goto error;
	}
	for(int i = 0; i < network->num_hidden_nodes; i++)
	{
		int k = i*network->num_output_nodes;
		for(int j = 0; j < network->num_output_nodes; j++)
		{
			if(fprintf(stream, "%f\t", network->output_layer.weights[k+j]) < 0)
				goto error;
		}
		if(fprintf(stream, "\n") < 0)
			goto error;
	}
	fclose(stream);
	return 0;
	error:
		fclose(stream);
		return -1;
}


int fbp_load_weights(nnet_t *network, char *datafile)
{
	FILE *stream = fopen(datafile, "rb");
	if(stream == NULL)
		return -1;

	float weight;
	for(int i = 0; i < network->num_input_nodes; i++)
	{
		int k = i*network->num_hidden_nodes;
		for(int j = 0; j < network->num_hidden_nodes; j++)
		{
			fscanf(stream, "%f", &weight);
			if(ferror(stream) || feof(stream))	
				goto error;
			network->hidden_layer.weights[k+j] = weight;
		}
	}

	for(int i = 0; i < network->num_hidden_nodes; i++)
	{
		int k =  i*network->num_output_nodes;
		for(int j = 0; j < network->num_output_nodes; j++)
		{
			fscanf(stream, "%f", &weight);
			if(ferror(stream) || feof(stream))	
				goto error;
			network->output_layer.weights[k+j] = weight;
		}
	}
	fclose(stream);
	return 0;
	error:
		fclose(stream);
		return -1;
}



/* Noise here refers to the making of edges of characters rough */
void fbp_add_noise(nnet_t *network)
{
	srand(time(NULL));
	int rnum, MARK = 5, OBJECT = 1;
	int blank_spots, num_marks;
	for(int i = 0; i < IMAGE_HEIGHT; i++)
	{
		int k = i*IMAGE_WIDTH;
		for(int j = 0; j < IMAGE_WIDTH; j++)
		{
			if(network->input_layer.outputs[k+j] == 1)
			{
				blank_spots = num_marks = 0;
				if(i-1 >= 0)
				{
					if(network->input_layer.outputs[(i-1)*IMAGE_WIDTH+j] == -1)
						blank_spots++;
					else if(network->input_layer.outputs[(i-1)*IMAGE_WIDTH+j] == MARK)
						num_marks++;
				}
				if(i+1 < IMAGE_HEIGHT)
				{
					if(network->input_layer.outputs[(i+1)*IMAGE_WIDTH+j] == -1)
						blank_spots++;
					else if(network->input_layer.outputs[(i+1)*IMAGE_WIDTH+j] == MARK)
						num_marks++;
				}
				if(j-1 >= 0)
				{
					if(network->input_layer.outputs[k+(j-1)] == -1)
						blank_spots++;
					else if(network->input_layer.outputs[k+(j-1)] == MARK)
						num_marks++;
				}
				if(j+1 < IMAGE_WIDTH)
				{
					if(network->input_layer.outputs[k+(j+1)] == -1)
						blank_spots++;
					else if(network->input_layer.outputs[k+(j+1)] == MARK)
						num_marks++;
				}
				if(i == 0 || j == 0 || i == IMAGE_HEIGHT-1 || j == IMAGE_WIDTH-1)	// edge pixels always have three neighbors
					blank_spots++;		
			
				if(blank_spots == 1 && num_marks == 0)
					network->input_layer.outputs[k+j] = MARK;
			}
			
		}
	}
	for(int i = 0; i < network->num_input_nodes; i++)
	{
		if(network->input_layer.outputs[i] == MARK)
		{
			rnum = rand()%2;
			if(rnum == 0)
				rnum = -1;
			network->input_layer.outputs[i] = rnum;
		}
	}

}