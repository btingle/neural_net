#include "pch.h"
#include "nnetapi.h"
#include <windows.h>
#include <tchar.h>

/*
 * File I/O for loading testing/training data for nnet class.
 * Would rather have it here than there, as it makes up a significant portion of the code.
 * Designed to read idx3-ubyte and idx1-ubyte file formats; documentation for format can be found here: http://yann.lecun.com/exdb/mnist/
*/

using namespace std;
using namespace arma;

char* nnet_io::load_data(string path) // loads raw data from idx3 or idx1 file and stores it in buffer
{
	ifstream data(path, ifstream::binary);
	if (!data.is_open())
	{
		return NULL;
	}
	data.seekg(0, data.end);
	int length = data.tellg();		   // gets length of file before read
	data.seekg(0, data.beg);

	//printf("%d length", length);

	char * buffer = new char[length];  // allocates buffer for read

	data.read(buffer, length);

	return buffer;
}

int nnet_io::concat_int(char* buf) // creates int value from array of 4 bytes
{
	int result = 0;
	for (int i = 0; i < 3; i++)
	{
		result = result | (0xFF & buf[i]);
		result = result << 8;
	}
	result = result | (0xFF & buf[3]);
	return result;
}

float nnet_io::concat_float(char* buf) // creates float value from array of 4 bytes
{
	int result = 0;
	for (int i = 0; i < 3; i++)
	{
		result = result | (0xFF & buf[i]);
		result = result << 8;
	}
	result = result | (0xFF & buf[3]);
	return (float)result; // this is apparently bad practice, but it works and I don't care
}

double nnet_io::concat_double(char* buf) // creates double value from array of 8 bytes
{
	long result = 0;
	for (int i = 0; i < 7; i++)
	{
		result = result | (0xFF & buf[i]);
		result = result << 8;
	}
	result = result | (0xFF & buf[7]);
	return (double)result;
}

int	nnet_io::load_items(string path, mat &target) // loads training or testing data matrix
{
	char* buffer = load_data(path);

	if (buffer == NULL) 
	{
		printf("Invalid filepath for items!\n");
		return 0;
	}

	int data_id = buffer[2];		// third byte of idx3 file contains data type for file
	int data_val = 1;

	switch (data_id)				// gets number of bytes for each datum
	{
	case 0x08: data_val = 1; break; // unsigned char 8  bits
	case 0x09: data_val = 1; break; // signed char   8  bits
	case 0x0b: data_val = 1; break; // short		 8  bits
	case 0x0c: data_val = 4; break; // int			 32 bits
	case 0x0d: data_val = 4; break; // float		 32 bits
	case 0x0e: data_val = 8; break; // double		 64 bits
	}

	int  dims_no = buffer[3];
	int* dims = new int[dims_no];
	for (int i = 0; i < dims_no; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			dims[i] = dims[i] | (0xFF & buffer[(i + 1) * 4 + j]);
			dims[i] = dims[i] << 8;
		}
		dims[i] = dims[i] >> 8;
		//printf("\n%d dim %d", dims[i], i);
	}
	// dims[0] will always be number of items. dims[1 -> n] will be dimensions of matrices, which we will ignore and just multiply together so we get a vertical slice for each item

	int num_items = dims[0];
	int slice_length = 1;
	for (int i = 1; i < dims_no; i++)
	{
		slice_length *= dims[i];
	}

	target = mat(slice_length, num_items); // allocates matrix
	char* datumbuf = new char[data_val];

	int colno = 0; // for counting number of columns in matrix. This does not necessarily line up with j iterator for buffer, as data type may be longer than 1 byte

	for (int i = 0; i < num_items; i++)
	{
		for (int j = 0; j < slice_length; )
		{
			for (int k = 0; k < data_val; j++)
			{
				datumbuf[k] = buffer[((dims_no + 1) * 4) + (i * slice_length) + j];
				k++;
			}
			switch (data_id)
			{
			case 0x08: target(colno, i) = datumbuf[0] / 255.0;	   break;	// converts char/short into floating point number from 0.0 -> 1.0, because we use the sigmoid function
			case 0x09: target(colno, i) = datumbuf[0] / 255.0;	   break;   // sidenote: make sure your compiler treats chars as unsigned by default. Neuron values are not meant to be negative, at least for this example
			case 0x0b: target(colno, i) = datumbuf[0] / 255.0;	   break;
			case 0x0c: target(colno, i) = concat_int(datumbuf);    break;	// not quite sure what to do with other data types, but I have support for them regardless
			case 0x0d: target(colno, i) = concat_float(datumbuf);  break;
			case 0x0e: target(colno, i) = concat_double(datumbuf); break;
			}
			colno++;
		}
		colno = 0;
	}

	delete[] buffer; // de-allocates char buffer once done

	printf("Finished loading items...");
	return 1;
}

int nnet_io::load_labels(string path, ivec &target) // loads labels from file and stores in vector
{
	char* buffer = load_data(path);

	if (buffer == NULL)
	{
		printf("Invalid filepath for labels!\n");
		return 0;
	}

	int data_id = buffer[2];		// third byte of idx3 file contains data type for file
	int data_val = 1;

	switch (data_id)				// gets number of bytes for each datum
	{
	case 0x08: data_val = 1; break; // unsigned char 8  bits
	case 0x09: data_val = 1; break; // signed char   8  bits
	case 0x0b: data_val = 1; break; // short		 8  bits
	case 0x0c: data_val = 4; break; // int			 32 bits
	case 0x0d: data_val = 4; break; // float		 32 bits
	case 0x0e: data_val = 8; break; // double		 64 bits
	}

	char* tempbuf = new char[4];
	for (int i = 0; i < 4; i++)
	{
		tempbuf[i] = buffer[4 + i];
	}
	int num_items = concat_int(tempbuf);

	target = ivec(num_items); // allocates vector
	char* datumbuf = new char[data_val];

	int colno = 0; // for counting current column in vector. Column no. does not necessarily line up with i, as data values may be longer than one byte

	for (int i = 8; i < (num_items * data_val) + 8; )
	{
		for (int k = 0; k < data_val; i++)
		{
			datumbuf[k] = buffer[i];
			k++;
		}
		switch (data_id)
		{
		case 0x08: target(colno) = datumbuf[0];			    break;	// doesn't convert char, because these are labels
		case 0x09: target(colno) = datumbuf[0];			    break;	
		case 0x0b: target(colno) = datumbuf[0];			    break;	
		case 0x0c: target(colno) = concat_int(datumbuf);    break;	// since labels are an ivec, the float and doubles here won't work (at least, not very well), not that they should really need to be used anyhow
		case 0x0d: target(colno) = concat_float(datumbuf);  break;	
		case 0x0e: target(colno) = concat_double(datumbuf); break;
		}
		colno++;
	}

	delete[] buffer; // de-allocates char buffer once done

	printf("Finished loading labels...");
	return 1;
}

int nnet_io::get_max_filename()
{
	WIN32_FIND_DATA FindFile;
	HANDLE			Find;

	Find = FindFirstFile(TEXT("nets/net_*"), &FindFile);

	if (Find == INVALID_HANDLE_VALUE)
	{
		return 1;
	}

	int f = ((FindFile.cFileName[4] & 0x0F) * 10) + (FindFile.cFileName[5] & 0x0F);
	while (FindNextFile(Find, &FindFile))
	{
		f = ((FindFile.cFileName[4] & 0x0F) * 10) + (FindFile.cFileName[5] & 0x0F);
	}

	return f + 1; // since files are sorted alphanumerically (I think) this should give a directory no. that hasn't been made yet
}


void nnet_io::get_save_dir(string &save_dir)
{
	int   max_dir    = get_max_filename();

	save_dir = "nets/net_" + to_string(max_dir / 10) + to_string(max_dir % 10);

	CreateDirectoryA(save_dir.c_str(), NULL);
}
