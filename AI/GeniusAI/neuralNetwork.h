

/*******************************************************************
* addapted by Trevor Standley for use as a function approximator
* Addapted from:
* Basic Feed Forward Neural Network Class
* ------------------------------------------------------------------
* Bobby Anguelov - takinginitiative.wordpress.com (2008)
* MSN & email: banguelov@cs.up.ac.za
********************************************************************/
#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H
//standard includes
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <limits>


class neuralNetwork
{
private:

	//number of neurons
	int nInput, nHidden1, nHidden2, nOutput;
	
	//neurons
	double* inputNeurons;
	double* hiddenNeurons1;
	double* hiddenNeurons2;
	double* outputNeurons;

	//weights
	double** wInputHidden;
	double** wHidden2Hidden;
	double** wHiddenOutput;
		

public:

	//constructor & destructor
	neuralNetwork(int numInput, int numHidden1, int numHidden2, int numOutput);
	neuralNetwork(const neuralNetwork&);
	neuralNetwork();
	void operator = (const neuralNetwork&);
	~neuralNetwork();

	//weight operations
	double* feedForwardPattern( double* pattern );
	void backpropigate(double* pattern, double OLR, double H2LR, double H1LR );

	void tweakWeights(double howMuch);
	void mate(const neuralNetwork&n1,const neuralNetwork&n2);



private: 
	void initializeWeights();
	inline double activationFunction( double x );
	void feedForward( double* pattern );
	
};

std::istream & operator >> (std::istream &, neuralNetwork & ann);
std::ostream & operator << (std::ostream &, const neuralNetwork & ann);
#endif