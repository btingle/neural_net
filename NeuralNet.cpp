#include "pch.h"
#include "nnetapi.h"
#include <math.h>

using namespace arma;
using namespace std;

/*
 * Neural net class for recognizing handwritten digits.
 * Thanks to 3blue1brown for the idea, as well as supplying link to the training/testing data
 * It should be noted that while this class is intended for the handwritten digits example, it can easily be modified to learn from any data set.
 */

class nnet
{
	mat				  training_input;   // matrix, to hold all training data
	ivec			  training_labels;	// vector, to hold all training labels
	mat				  testing_input;    // matrix, to hold all testing data
	ivec			  testing_labels;	// vector, to hold all testing labels
	vec				  output;			// column vector, just one output
	vec*			  hidden;			// pointer to array of hidden layers
	vec*			  biases;			// pointer to array of biases
	mat*			  weights;			// pointer to array of weight matrices
	vec*			  biases_gradient;	// pointer to array of the bias gradient values
	mat*			  weight_gradient;  // pointer to array of the weight matrix values
	int				  hl;				// number of hidden layers
	string			  train_items_fp;	// path to training data
	string			  train_labels_fp;  // path to training labels
	string			  test_items_fp;	// path to testing data
	string			  test_labels_fp;	// path to testing labels
	string		      save_dir;			// path to directory to save weights and biases to

	// I know biases and weights can be combined into one, but separating them is more easily comprehensible and doesn't require any weirdness

	static void sigmoid(double &x);

	static void sigmoid_prime(double &x);

	public:

		nnet(int hidden_h, int hidden_w, int insize, int outsize); // creates new network and save file

		nnet(string load_dir); // loads old network from save directory

		~nnet();

		void update_filepath(string training_items_path, string training_labels_path, string testing_items_path, string testing_labels_path);

		void print_filepath();

		void train(int iterations);

		void apply_gradient();

		void test();

		int get_output();

		void save_net();

		double learn_rate; // learning rate of network. I want this one to be public as it should be able to be changed on the fly

};

nnet::nnet(int hidden_h, int hidden_w, int outsize, int insize) // creates a fresh neural net
{
	nnet_io::get_save_dir(save_dir); // creates a new directory for files to be saved to
	train_items_fp  = " ";
	train_labels_fp = " ";
	test_items_fp	= " ";
	test_labels_fp  = " ";

	hl	= hidden_w;
	
	learn_rate = 0.002; // learnrate should be small enough to avoid overfitting, but large enough to learn at a decent rate
	weights			= new mat[hidden_w + 1];
	biases			= new vec[hidden_w + 1];
	weight_gradient = new mat[hidden_w + 1];
	biases_gradient	= new vec[hidden_w + 1];
	hidden			= new vec[hidden_w];

	/* Initializes weight/bias/gradient matrices
	 * A note on the initialization values for weights:
	 * A little birdie told me that, in a sigmoid function activated network, your weights should range from (-r, r),
	 * Where r = 4 * sqrt(6 / (in + out))	// in=incoming neurons, out=outgoing neurons
	 * Since we need to modify our randu to range from (-1, 1) by subtracting .5 and multiplying by 2, this simplifies to what you see below
	 * I have little idea why the weights should initially be set this way, but I'm going to trust the scientists on this one
	*/
	weights[0]		   = (randu<mat>(hidden_h, insize) - 0.5) * 8.0 * sqrt(6.0 / (insize + hidden_h)); // first term is height, second is width
	weight_gradient[0] = zeros<mat>(hidden_h, insize);
	biases[0]		   = zeros<vec>(hidden_h);			// "vec" class is column vector by default
	biases_gradient[0] = zeros<vec>(hidden_h);
	hidden[0]		   = zeros<vec>(hidden_h);
	for (int i = 1; i < hidden_w; i++) // there must be at least ONE hidden layer in this network... so don't use it on too simple a function, or it will overfit
	{
		weights[i]			= (randu<mat>(hidden_h, hidden_h) - 0.5) * 8.0 * sqrt(6.0 / (hidden_h + hidden_h));
		weight_gradient[i]  = zeros<mat>(hidden_h, hidden_h);
		biases[i]			= zeros<vec>(hidden_h);
		biases_gradient[i]	= zeros<vec>(hidden_h);
		hidden[i]			= zeros<vec>(hidden_h);
	}
	weights[hidden_w]		  = (randu<mat>(outsize, hidden_h) - 0.5) * 8.0 * sqrt(6.0 / (hidden_h + outsize));
	weight_gradient[hidden_w] = zeros<mat>(outsize, hidden_h);
	biases[hidden_w]		  = zeros<vec>(outsize);
	biases_gradient[hidden_w] = zeros<vec>(outsize);
	output					  = zeros<vec>(outsize);

}

nnet::nnet(string load_dir) // a bit messy, but it works
{
	ifstream net_info (load_dir + "/net_info.txt");
	string tmp;
	save_dir = load_dir; // will make sure this net is saved to the same folder it is loaded from

	getline(net_info, tmp); // gets numerical components
	hl = stoi(tmp, nullptr);
	getline(net_info, tmp);
	learn_rate = stof(tmp, nullptr);

	getline(net_info, train_items_fp);
	getline(net_info, train_labels_fp);
	getline(net_info, test_items_fp);
	getline(net_info, test_labels_fp);

	net_info.close();

	weights				= new mat[hl + 1];
	biases				= new vec[hl + 1];
	weight_gradient		= new mat[hl + 1];
	biases_gradient		= new vec[hl + 1];
	hidden			    = new vec[hl];

	for (int i = 0; i < hl + 1; i++)
	{
		string bias_fn   = load_dir + "/bias_"   + to_string(i / 10) + to_string(i % 10);
		string weight_fn = load_dir + "/weight_" + to_string(i / 10) + to_string(i % 10);

		biases		   [i].load(bias_fn);
		weights		   [i].load(weight_fn);
		weight_gradient[i].load(weight_fn);
		weight_gradient[i].fill(0);
		biases_gradient[i].load(bias_fn);
		biases_gradient[i].fill(0);
	}
	for (int i = 0; i < hl; i++)
	{
		hidden[i] = zeros<vec>(weights[i].n_rows);
	}
	output = zeros<vec>(weights[hl].n_rows);
}

void nnet::save_net()
{
	string net_info = to_string(hl) + "\n" // width of hidden layers
		+ to_string(learn_rate) + "\n" // learning rate
		+ train_items_fp + "\n" // stores filepaths for data
		+ train_labels_fp + "\n"
		+ test_items_fp + "\n"
		+ test_labels_fp + "\n";

	ofstream net_info_f;
	net_info_f.open(save_dir + "/net_info.txt");
	net_info_f << net_info;

	for (int i = 0; i < hl + 1; i++)
	{
		string bias_fn = save_dir + "/bias_" + to_string(i / 10) + to_string(i % 10);
		string weight_fn = save_dir + "/weight_" + to_string(i / 10) + to_string(i % 10);
		biases[i].save(bias_fn);
		weights[i].save(weight_fn);
	}
}

nnet::~nnet()
{	
	save_net();
}

void nnet::sigmoid(double& x) // activation function
{
	x = 1.0 / (1.0 + exp(-x));
}

void nnet::sigmoid_prime(double& x) // derivative of activation function
{
	x = x * (1.0 - x);
}

void nnet::update_filepath(string training_items_path, string training_labels_path, string testing_items_path,  string testing_labels_path)
{
	train_items_fp  = training_items_path;
	train_labels_fp = training_labels_path;
	test_items_fp   = testing_items_path;
	test_labels_fp  = testing_labels_path;
}

void nnet::print_filepath()
{
	cout << train_items_fp << train_labels_fp << test_items_fp << test_labels_fp << "\n";
}

void nnet::train(int iterations)
{
	printf("Loading Training Data...");
	if (nnet_io::load_items(train_items_fp, training_input) &&  // loads in training examples from idx-ubyte files
		nnet_io::load_labels(train_labels_fp, training_labels))
	{
		printf("Done!\n");
		if (training_input.n_cols != training_labels.n_rows)
		{
			printf("Labels do not match data!");
			return;
		}
		printf("\nImage size: %d\nImage items: %d \nLabel items: %d\n", training_input.n_rows, training_input.n_cols, training_labels.n_rows);
	}
	else
	{
		return;
	}

	int no_correct = 0;
	int no_correct_old = 0;
	int d_correct = 0;
	int target_ind = 0;
	vec input;
	vec cost;
	vec sigma;
	vec target = zeros<vec>(output.n_rows);

	printf("Training... 0 / %d Complete.", iterations);

	for (int k = 0; k < iterations; k++) // don't mind the mismatched iterators
	{

		for (int i = 0; i < training_input.n_cols; i++)
		{
			input = training_input.col(i);
			target_ind = training_labels(i) & 0x0F - 1; // training_labels contains ascii '0'-'9' values, since our outputs range from 0-9, we can just do a simple bitwise on them
			target(target_ind) = 1.0; // we want index of target that matches desired output to be 1.0
			
			/* Forward Propogation */
			hidden[0] = weights[0] * input;			  // multiply together, then...
			hidden[0] += biases[0];					  // ...add biases, then...
			hidden[0] = hidden[0].for_each(sigmoid);  // ...activate neurons

			for (int j = 1; j < hl; j++)
			{
				hidden[j] = weights[j] * hidden[j - 1];   // multiply together, then...
				hidden[j] += biases[j];				      // ...add biases, then...
				hidden[j] = hidden[j].for_each(sigmoid);  // ...activate neurons, repeat until we're out of hidden layers
			}

			output = weights[hl] * hidden[hl - 1];    // multiply together, then...
			output += biases[hl];					  // ...add biases, then...
			output = output.for_each(sigmoid);		  // ...activate neurons, this is the output for the network

			/* Backward Propogation */
			/* Whereas the previous section of code was fairly intuitive, this section is decidedly less so.
			 * I'll attempt to give some explanation as to what I'm doing in the code, but if you don't know much about backpropogation and want to understand I will link some sources below
			 * https://arxiv.org/pdf/1802.01528.pdf <-- useful pdf covering vector chain rule and backpropogation calculus
			 * https://youtu.be/zpykfC4VnpM <-- useful video for neural net math
			 * https://youtu.be/tIeHLnjs5U8 <-- great explanation of chain rule calculus for neural networks
			 * There are many other sources online, I am just listing a few that I found especially useful
			 */

			cost = output - target; // derivative of cost function
			no_correct += output.index_max() == target_ind ? 1 : 0;

			// some notes:
			// derivative of sigmoid function is: x * (1 - x)
			// % refers to the schur product, which is elementwise multiplication of matrices

			output = output.for_each(sigmoid_prime);		   // output = output'
			sigma = output % cost;							   // sigma = output' % cost
			weight_gradient[hl] -= sigma * hidden[hl - 1].t(); // dC/dW = sigma * neurons.T		//"neurons" here refers to the last hidden layer in the network
			biases_gradient[hl] -= sigma;					   // dC/dB = sigma

			for (int j = hl - 1; j > 0; j--)
			{
				hidden[j] = hidden[j].for_each(sigmoid_prime);			// neurons = neurons'
				sigma = hidden[j] % (weights[j + 1].t() * sigma);		// sigma = neurons' % (weights[+1].T * sigma[-1])
				weight_gradient[j] -= sigma * hidden[j - 1].t();		// dC/dW = sigma * neurons[-1].T
				biases_gradient[j] -= sigma;							// dC/dB = sigma
			}

			hidden[0] = hidden[0].for_each(sigmoid_prime);	   // neurons = neurons'
			sigma = hidden[0] % (weights[1].t() * sigma);	   // sigma = neurons' % (weights[1].T * sigma[-1])
			weight_gradient[0] -= sigma * input.t();		   // dC/dW = sigma * input.T
			biases_gradient[0] -= sigma;					   // dC/dB = sigma

			target.fill(0);

		}

		apply_gradient();

		if (k > 0)
		{
			d_correct += no_correct - no_correct_old;
		}

		printf("\rTraining... %d / %d Complete. Avg Correct: %d, %d%%, Avg change: %f", k + 1, iterations, no_correct, no_correct*100/training_input.n_cols, d_correct / (float)k);

		if ((k + 1) % 500 == 0)
		{
			save_net(); // auto saves progress every 500 iterations
		}

		no_correct_old = no_correct;
		no_correct = 0;
	}

	training_input.reset();

}

void nnet::apply_gradient()
{
	// The cost gradient is the accumulation of all derivatives in the network with respect to the cost, averaged between all training examples and multiplied by the learning rate
	for (int i = 0; i < hl + 1; i++) 
	{
		weights[i] += learn_rate * (weight_gradient[i] / training_input.n_cols);
		biases [i] += learn_rate * (biases_gradient[i] / training_input.n_cols);

		weight_gradient[i].fill(0);
		biases_gradient[i].fill(0);
	}
}

void nnet::test()
{
	printf("Loading Testing Data...");
	if (nnet_io::load_items(test_items_fp, testing_input) &&  // loads in training examples from idx-ubyte files
		nnet_io::load_labels(test_labels_fp, testing_labels))
	{
		printf("Done!\n");
		if (testing_input.n_cols != testing_labels.n_rows)
		{
			printf("Labels do not match data!");
			return;
		}
		printf("\nImage size: %d\nImage items: %d \nLabel items: %d\n", testing_input.n_rows, testing_input.n_cols, testing_labels.n_rows);
	}

	vec input;
	int no_correct = 0;
	int target_ind = 0;

	for (int i = 0; i < testing_input.n_cols; i++)
	{
		input = testing_input.col(i);
		target_ind = testing_labels(i) & 0x0F - 1; // training_labels contains ascii '0'-'9' values, since our outputs range from 0-9, we can just do a simple bitwise on them

		/* Forward Propogation */
		hidden[0] = weights[0] * input;			  // multiply together, then...
		hidden[0] += biases[0];					  // ...add biases, then...
		hidden[0] = hidden[0].for_each(sigmoid);  // ...activate neurons

		for (int j = 1; j < hl; j++)
		{
			hidden[j] = weights[j] * hidden[j - 1];   // multiply together, then...
			hidden[j] += biases[j];				      // ...add biases, then...
			hidden[j] = hidden[j].for_each(sigmoid);  // ...activate neurons, repeat until we're out of hidden layers
		}

		output = weights[hl] * hidden[hl - 1];    // multiply together, then...
		output += biases[hl];					  // ...add biases, then...
		output = output.for_each(sigmoid);		  // ...activate neurons, this is the output for the network

		no_correct += output.index_max() == target_ind ? 1 : 0;
	}

	printf("No. correct: %d, %d%%\n", no_correct, (no_correct * 100) / testing_input.n_cols);
}

int nnet::get_output() // need to implement this
{
	return 2; // it'll be right 10% of the time
}

int main() // this console interface is a bit jank, but it works correctly
{
	char in = 0;
	while (in != 'q')
	{
		in = 'q';
		cout << "Enter 'n' for new net\nEnter 'l to load existing\nEnter 'q' to quit\n";
		cin  >> in;
		if (in == 'n')
		{
			int hh, hw, ot, in = 0;
			cout << "Enter hidden layer height, width, output size, and input size.\n";
			cin  >> hh >> hw >> ot >> in;
			nnet Net = nnet(hh, hw, ot, in);

			string trainifp, trainlfp, testifp, testlfp;
			cout << "Enter filepaths for: training items, training labels, testing items, testing labels, in that order.\n";
			cin  >> trainifp >> trainlfp >> testifp >> testlfp;
			Net.update_filepath(trainifp, trainlfp, testifp, testlfp);

			int its = 0;
			cout << "How many iterations would you like to train for?\n";
			cin  >> its;
			Net.train(its);
			cout << "\n";
		}
		if (in == 'l')
		{
			string fn;
			cout << "Enter directory of net (relative to working directory)\n";
			cin  >> fn;
			nnet Net = nnet(fn);

			char inl = 0;

			while (inl != 'q')
			{
				cout << "'t' to train, 'c' to change params, 'o' to test, 'q' to quit\n";
				cin >> inl;

				if (inl == 't')
				{
					int its = 0;
					cout << "How many iterations would you like to train for?\n";
					cin >> its;
					Net.train(its);
					cout << "\n";
				}
				if (inl == 'c')
				{
					char inlc = 0;
					cout << "What would you like to change? 'l' for learning rate, 'f' for filepath\n";
					cin >> inlc;
					if (inlc == 'l')
					{
						cout << "Current learnrate: " << Net.learn_rate << "\n";
						cout << "Enter new learnrate\n";
						cin >> Net.learn_rate;
					}
					if (inlc == 'f')
					{
						Net.print_filepath();
						string trainifp, trainlfp, testifp, testlfp;
						cout << "Enter filepaths for: training items, training labels, testing items, testing labels, in that order.\n";
						cin >> trainifp >> trainlfp >> testifp >> testlfp;
						Net.update_filepath(trainifp, trainlfp, testifp, testlfp);
					}
				}
				if (inl == 'o')
				{
					Net.test();
				}
			}
			
		}
	}
}