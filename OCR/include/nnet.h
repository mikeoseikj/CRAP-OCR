#ifndef NNET_H
#define NNET_H

typedef struct 
{
	// Don't access num_..._nodes directly
	int num_input_nodes;
	int num_hidden_nodes;
	int num_output_nodes;
	
	float learn_rate;
	float momentum;

	struct
	{
		float *inputs;
		float *outputs; 
	}input_layer;

	struct 
	{
		float *inputs;
		float *outputs;
		float *weights;
		float *errors;

		// for momemtum aspect to speed up training
		float *prev_weights;
	}hidden_layer;

	struct output_layer
	{
		float *inputs;
		float *outputs;
		float *weights;
		float *errors;
		float *expected;

		// for momemtum aspect to speed up training
		float *prev_weights;
	}output_layer;



}nnet_t;


static float sigmoid(float value);
nnet_t *fbp_create_network(int num_input_neurons, int num_hidden_neurons, int num_output_neurons);
void fbp_set_random_weights(nnet_t *network);
void fbp_set_train_inputs(nnet_t *network, float input[], float expected[]);
void fbp_set_run_inputs(nnet_t *network, float input[]);
void fbp_forward_propagate(nnet_t *network);
void fbp_backward_propagate(nnet_t *network, float *error);
void fbp_update_weights(nnet_t *network);
int fbp_store_weights(nnet_t *network, char *datafile);
int fbp_load_weights(nnet_t *network, char *datafile);
void fbp_add_noise(nnet_t *network);


#endif
