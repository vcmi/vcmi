#include <stdlib.h>
#include <string.h>

#include "neuralNetwork.h"
//using namespace std;

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

static float norm(void)//add desired mean, multiply to get desired SD
{
	static float kept = 0;
	static bool in = 0;
	if(!in)
	{
		float x = (rand()+1)/float(RAND_MAX+1); 
		float f = sqrtf( - 2.0f * log(x) );
			  x = (rand()+1)/float(RAND_MAX+1);
		kept = f * cosf( 2.0f * M_PI * x );
		in = true;
		return f * sinf( 2.0f * M_PI * x );
	}
	else
	{
		in = false;
		return kept;
	}
}

/*******************************************************************
* Constructors
********************************************************************/
neuralNetwork::neuralNetwork() : nInput(0), nHidden1(0), nHidden2(0), nOutput(0)
{
	inputNeurons = new double[1] ;
	hiddenNeurons1 = new double[1] ;
	hiddenNeurons2 = new double[1] ;
	outputNeurons = new double[1] ;
	wInputHidden = new double*[1] ;
	wInputHidden[0] = new double[1];
	wHidden2Hidden = new double*[1] ;
	wHidden2Hidden[0] = new (double[1]);	
	wHiddenOutput = new double*[1] ;
	wHiddenOutput[0] = new double[1];		
}

neuralNetwork::neuralNetwork(const neuralNetwork& other): nInput(0), nHidden1(0), nHidden2(0), nOutput(0)
{
	inputNeurons = new double[1] ;
	hiddenNeurons1 = new double[1] ;
	hiddenNeurons2 = new double[1] ;
	outputNeurons = new double[1] ;
	wInputHidden = new double*[1] ;
	wInputHidden[0] = new double[1];
	wHidden2Hidden = new double*[1] ;
	wHidden2Hidden[0] = new (double[1]);	
	wHiddenOutput = new double*[1] ;
	wHiddenOutput[0] = new double[1];		
	*this = other;
}

neuralNetwork::neuralNetwork(int nI, int nH1, int nH2, int nO) : nInput(nI), nHidden1(nH1), nHidden2(nH2), nOutput(nO)
{				
	//create neuron lists
	//--------------------------------------------------------------------------------------------------------
	inputNeurons = new double[nInput + 1] ;
	for ( int i=0; i < nInput; i++ ) inputNeurons[i] = 0;

	//create input bias neuron
	inputNeurons[nInput] = -1;

	hiddenNeurons1 = new double[nHidden1 + 1] ;
	for ( int i=0; i < nHidden1; i++ ) hiddenNeurons1[i] = 0;

	//create hidden bias neuron
	hiddenNeurons1[nHidden1] = -1;

	hiddenNeurons2 = new double[nHidden2 + 1] ;
	for ( int i=0; i < nHidden2; i++ ) hiddenNeurons2[i] = 0;

	//create hidden bias neuron
	hiddenNeurons2[nHidden2] = -1;

	outputNeurons = new double[nOutput] ;
	for ( int i=0; i < nOutput; i++ ) outputNeurons[i] = 0;

	//create weight lists (include bias neuron weights)
	//--------------------------------------------------------------------------------------------------------
	wInputHidden = new double*[nInput + 1] ;
	for ( int i=0; i <= nInput; i++ ) 
	{
		wInputHidden[i] = new double[nHidden1];
		for ( int j=0; j < nHidden1; j++ ) wInputHidden[i][j] = 0;		
	}

	wHidden2Hidden = new double*[nHidden1 + 1] ;
	for ( int i=0; i <= nHidden1; i++ ) 
	{
		wHidden2Hidden[i] = new (double[nHidden2]);
		for ( int j=0; j < nHidden2; j++ ) wHidden2Hidden[i][j] = 0;		
	}

	wHiddenOutput = new double*[nHidden2 + 1] ;
	for ( int i=0; i <= nHidden2; i++ ) 
	{
		wHiddenOutput[i] = new double[nOutput];			
		for ( int j=0; j < nOutput; j++ ) wHiddenOutput[i][j] = 0;		
	}	
	
	//initialize weights
	//--------------------------------------------------------------------------------------------------------
	initializeWeights();			
}


void neuralNetwork::operator = (const neuralNetwork&cpy)//assumes same structure
{
	if(	nInput != cpy.nInput ||	nHidden1 != cpy.nHidden1 ||	nHidden2 != cpy.nHidden2 ||	nOutput != cpy.nOutput)
	{
		delete[] inputNeurons;
		delete[] hiddenNeurons1;
		delete[] hiddenNeurons2;
		delete[] outputNeurons;

		//delete weight storage
		for (int i=0; i <= nInput; i++) delete[] wInputHidden[i];
		delete[] wInputHidden;

		for (int j=0; j <= nHidden2; j++) delete[] wHiddenOutput[j];
		delete[] wHiddenOutput;	

		for (int j=0; j <= nHidden1; j++) delete[] wHidden2Hidden[j];
		delete[] wHidden2Hidden;

		nInput = cpy.nInput;
		nHidden1 = cpy.nHidden1;
		nHidden2 = cpy.nHidden2;
		nOutput = cpy.nOutput;

		inputNeurons = new double[nInput + 1] ;
		inputNeurons[nInput] = -1;

		hiddenNeurons1 = new double[nHidden1 + 1] ;
		hiddenNeurons1[nHidden1] = -1;

		hiddenNeurons2 = new double[nHidden2 + 1] ;
		hiddenNeurons2[nHidden2] = -1;

		outputNeurons = new double[nOutput] ;

		//create weight lists (include bias neuron weights)
		//--------------------------------------------------------------------------------------------------------
		wInputHidden = new double*[nInput + 1] ;
		for ( int i=0; i <= nInput; i++ ) 
			wInputHidden[i] = new double[nHidden1];	

		wHidden2Hidden = new double*[nHidden1 + 1] ;
		for ( int i=0; i <= nHidden1; i++ ) 
			wHidden2Hidden[i] = new (double[nHidden2]);		

		wHiddenOutput = new double*[nHidden2 + 1] ;
		for ( int i=0; i <= nHidden2; i++ ) 
			wHiddenOutput[i] = new double[nOutput];				
	}

	for ( int i=0; i < nInput; i++ ) inputNeurons[i] = cpy.inputNeurons[i];
	for ( int i=0; i < nHidden1; i++ ) hiddenNeurons1[i] = cpy.hiddenNeurons1[i];
	for ( int i=0; i < nHidden2; i++ ) hiddenNeurons2[i] = cpy.hiddenNeurons2[i];
	for ( int i=0; i < nOutput; i++ ) outputNeurons[i] = cpy.outputNeurons[i];
	
	for ( int i=0; i <= nInput; i++ ) 
		for ( int j=0; j < nHidden1; j++ )
			wInputHidden[i][j] = cpy.wInputHidden[i][j];		
	
	for ( int i=0; i <= nHidden1; i++ ) 
		for ( int j=0; j < nHidden2; j++ )
			wHidden2Hidden[i][j] = cpy.wHidden2Hidden[i][j];		

	for ( int i=0; i <= nHidden2; i++ ) 	
		for ( int j=0; j < nOutput; j++ )
			wHiddenOutput[i][j] = cpy.wHiddenOutput[i][j];
	
}
/*******************************************************************
* Destructor
********************************************************************/
neuralNetwork::~neuralNetwork()
{
	//delete neurons
	delete[] inputNeurons;
	delete[] hiddenNeurons1;
	delete[] hiddenNeurons2;
	delete[] outputNeurons;

	//delete weight storage
	for (int i=0; i <= nInput; i++) delete[] wInputHidden[i];
	delete[] wInputHidden;

	for (int j=0; j <= nHidden2; j++) delete[] wHiddenOutput[j];
	delete[] wHiddenOutput;	

	for (int j=0; j <= nHidden1; j++) delete[] wHidden2Hidden[j];
	delete[] wHidden2Hidden;	
}

double* neuralNetwork::feedForwardPattern(double *pattern)
{
	feedForward(pattern);


	return outputNeurons;
}

void neuralNetwork::mate(const neuralNetwork&n1,const neuralNetwork&n2)
{
	for(int i = 0; i <= nInput; i++)
	{		
		for(int j = 0; j < nHidden1; j++) 
		{
			if(rand()%2==0)
				wInputHidden[i][j] = n1.wInputHidden[i][j];
			else
				wInputHidden[i][j] = n2.wInputHidden[i][j];
		}
	}
	for(int i = 0; i <= nHidden1; i++)
	{		
		for(int j = 0; j < nHidden2; j++) 
		{
			if(rand()%2==0)
				wHidden2Hidden[i][j] =n1.wHidden2Hidden[i][j];
			else
				wHidden2Hidden[i][j] =n2.wHidden2Hidden[i][j];	
		}
	}

	for(int i = 0; i <= nHidden2; i++)
	{		
		for(int j = 0; j < nOutput; j++) 
		{
			if(rand()%2==0)
				wHiddenOutput[i][j] =n1.wHiddenOutput[i][j];
			else
				wHiddenOutput[i][j] =n2.wHiddenOutput[i][j];	
		}
	}
}

void neuralNetwork::tweakWeights(double howMuch)
{
	//set range
	double rH = 1/sqrt( (double) nInput);
	double rO = 1/sqrt( (double) nHidden1);
	
	for(int i = 0; i <= nInput; i++)
	{		
		for(int j = 0; j < nHidden1; j++) 
		{
			wInputHidden[i][j] += howMuch*norm();	
		}
	}
	for(int i = 0; i <= nHidden1; i++)
	{		
		for(int j = 0; j < nHidden2; j++) 
		{
			wHidden2Hidden[i][j] += howMuch*norm();	
		}
	}

	for(int i = 0; i <= nHidden2; i++)
	{		
		for(int j = 0; j < nOutput; j++) 
		{
			wHiddenOutput[i][j] += howMuch* norm();
		}
	}
	//initializeWeights();
}

void neuralNetwork::initializeWeights()
{
	//set range
	double rH = 2.0/sqrt( (double) nInput);
	double rO = 2.0/sqrt( (double) nHidden1);
	
	//set weights between input and hidden 		
	//--------------------------------------------------------------------------------------------------------
	for(int i = 0; i <= nInput; i++)
	{		
		for(int j = 0; j < nHidden1; j++) 
		{
			//set weights to random values
			wInputHidden[i][j] =  norm()* rH;			
		}
	}
	
	for(int i = 0; i <= nHidden1; i++)
	{		
		for(int j = 0; j < nHidden2; j++) 
		{
			//set weights to random values
			wHidden2Hidden[i][j] = norm()* rO;			
		}
	}

	//set weights between hidden and output
	//--------------------------------------------------------------------------------------------------------
	for(int i = 0; i <= nHidden2; i++)
	{		
		for(int j = 0; j < nOutput; j++) 
		{
			//set weights to random values
			wHiddenOutput[i][j] = norm()* rO;
		}
	}
}
/*******************************************************************
* Activation Function
********************************************************************/
inline double neuralNetwork::activationFunction( double x )
{
	//sigmoid function
	return 1/(1+exp(-x));
}	

/*******************************************************************
* Feed Forward Operation
********************************************************************/
void neuralNetwork::feedForward(double* pattern)
{
	//set input neurons to input values
	for(int i = 0; i < nInput; i++) inputNeurons[i] = pattern[i];
	
	//Calculate Hidden Layer values - include bias neuron
	//--------------------------------------------------------------------------------------------------------
	for(int j=0; j < nHidden1; j++)
	{
		//clear value
		hiddenNeurons1[j] = 0;				
		
		//get weighted sum of pattern and bias neuron
		for( int i=0; i <= nInput; i++ ) hiddenNeurons1[j] += inputNeurons[i] * wInputHidden[i][j];
		
		//set to result of sigmoid
		hiddenNeurons1[j] = activationFunction( hiddenNeurons1[j] );
	}

	for(int j=0; j < nHidden2; j++)
	{
		//clear value
		hiddenNeurons2[j] = 0;				
		
		//get weighted sum of pattern and bias neuron
		for( int i=0; i <= nHidden1; i++ ) hiddenNeurons2[j] += hiddenNeurons1[i] * wHidden2Hidden[i][j];
		
		//set to result of sigmoid
		hiddenNeurons2[j] = activationFunction( hiddenNeurons2[j] );
	}

	//Calculating Output Layer values - include bias neuron
	//--------------------------------------------------------------------------------------------------------
	for(int k=0; k < nOutput; k++)
	{
		//clear value
		outputNeurons[k] = 0;				
		
		//get weighted sum of pattern and bias neuron
		for( int j=0; j <= nHidden2; j++ ) outputNeurons[k] += hiddenNeurons2[j] * wHiddenOutput[j][k];
		
		//set to result of sigmoid
		//outputNeurons[k] = activationFunction( outputNeurons[k] );
	}
}

void neuralNetwork::backpropigate(double* pattern, double OLR, double H2LR, double H1LR )
{
	//inputError = new double[nInput + 1] ;
	double * hiddenError1 = new double[nHidden1 + 1] ;
	double * hiddenError2 = new double[nHidden2 + 1] ;
	double * outputError = new double[nOutput] ;
	memset(hiddenError1,0,sizeof(double)*nHidden1);
	memset(hiddenError2,0,sizeof(double)*nHidden2);

	for(int i = 0; i < nOutput; i++)
	{
		outputError[i] = (pattern[i]-outputNeurons[i]);//*(outputNeurons[i]*(1-outputNeurons[i]));
		for(int ii = 0; ii <= nHidden2;ii++)
			hiddenError2[ii]+=outputError[i]*wHiddenOutput[ii][i];
		for(int ii = 0; ii <= nHidden2;ii++)
			wHiddenOutput[ii][i]+=OLR*hiddenNeurons2[ii]*outputError[i];
		
	}

	for(int i = 0; i < nHidden2; i++)
	{
		hiddenError2[i] *= (hiddenNeurons2[i]*(1-hiddenNeurons2[i]));
		for(int ii = 0; ii <= nHidden1;ii++)
			hiddenError1[ii]+=hiddenError2[i]*wHidden2Hidden[ii][i];
		for(int ii = 0; ii <= nHidden1;ii++)
			wHidden2Hidden[ii][i]+=H2LR*hiddenNeurons1[ii]*hiddenError2[i];
		
	}

	for(int i = 0; i < nHidden1; i++)
	{
		hiddenError1[i] *= (hiddenNeurons1[i]*(1-hiddenNeurons1[i]));

		for(int ii = 0; ii <= nInput;ii++)
			wInputHidden[ii][i]+=H1LR*inputNeurons[ii]*hiddenError1[i];
		
	}

	
	delete [] hiddenError1;
	delete [] hiddenError2;
	delete [] outputError;
}
