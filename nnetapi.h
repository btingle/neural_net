#ifndef NNET_API
#define NNET_API

#include "pch.h"
#include <iostream>
//#include <fstream>
#include <armadillo>
#include <string.h>

using namespace std;
using namespace arma;

// I/O class for reading in training/testing data files and storing it into training/testing matrices and vectors
class nnet_io
{
	private:

		static int concat_int(char* buf);

		static float  concat_float(char* buf);

		static double concat_double(char* buf);

		static char* load_data(string path);

	public:

		static int load_items(string path, arma::mat &target);

		static int load_labels(string path, arma::ivec &target);

		static int get_max_filename();

		static void get_save_dir(string &save_dir);
};

#endif