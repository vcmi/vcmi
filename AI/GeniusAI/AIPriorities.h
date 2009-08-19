#ifndef AIP_H
#define AIP_H

#include <string>
#include "CGeniusAI.h"
#include "neuralNetwork.h"

namespace GeniusAI {

class Network
{
public:
	Network();
	Network(vector<unsigned int> whichFeatures);// random network
	Network(istream & input);
	vector<unsigned int> whichFeatures;
	float feedForward(const vector<float> & stateFeatures);
	neuralNetwork net;					//a network with whichFeatures.size() inputs, and 1 output
};


class Priorities
{
public:
	Priorities();								//random brain
	Priorities(const string & filename);	//read brain from file


	vector<float> stateFeatures;
	int specialFeaturesStart;
	int numSpecialFeatures;
	void fillFeatures(const CGeniusAI::HypotheticalGameState & AI);
	float getValue(const CGeniusAI::AIObjective & obj);
	float getCost(vector<int> &resourceCosts,const CGHeroInstance * moved,int distOutOfTheWay);
	vector<vector<Network> > objectNetworks;
};

}
#endif