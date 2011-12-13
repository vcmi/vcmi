#pragma once

#include "CGeniusAI.h"
#include "neuralNetwork.h"

namespace geniusai {

class Network
{
public:
	Network();
	Network(vector<ui32> whichFeatures);// random network
	Network(istream & input);
	vector<ui32> whichFeatures;
	double feedForward(const vector<double> & stateFeatures);
	neuralNetwork net;					//a network with whichFeatures.size() inputs, and 1 output
};


class Priorities
{
public:
	Priorities(const string & filename);	//read brain from file


	vector<double> stateFeatures;
	int specialFeaturesStart;
	int numSpecialFeatures;
	void fillFeatures(const CGeniusAI::HypotheticalGameState & AI);
	double getValue(const CGeniusAI::AIObjective & obj);
	double getCost(vector<int> &resourceCosts,const CGHeroInstance * moved,int distOutOfTheWay);
	vector<vector<Network> > objectNetworks;
	vector<map<int,Network> > buildingNetworks;
};

}