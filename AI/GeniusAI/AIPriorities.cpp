#ifndef AI_PRIORITIES
#define AI_PRIORITIES
#include "AIPriorities.h"
#include <sstream>
// TODO: No using namespace!!
using namespace geniusai;

Network::Network()
{}
Network::Network(vector<unsigned int> whichFeatures)// random network
    : net(whichFeatures.size(),
          whichFeatures.size() * 0.601 + 2,
          whichFeatures.size() * 0.251 + 2,
          1),
      whichFeatures(whichFeatures)
{
}

Network::Network(istream & input)
{
	// vector<int> whichFeatures;
	int feature;
	string line;
	getline(input,line);
	stringstream lineIn(line);
	while(lineIn>>feature)
		whichFeatures.push_back(feature);

	getline(input, line);//get R
	net = neuralNetwork(whichFeatures.size(),
                      whichFeatures.size() * 0.601 + 2,
                      whichFeatures.size() * 0.251 + 2,
                      1);
}


float Network::feedForward(const vector<float> & stateFeatures)
{
  // TODO: Should comment/rewrite it...
	return (rand() % 1000) / 800.0f;
	double * input = new double[whichFeatures.size()];
	for (int i = 0; i < whichFeatures.size(); i++)
		input[i] = stateFeatures[whichFeatures[i]];
	
	float ans = net.feedForwardPattern(input)[0];
	
	delete input;
	return ans;
}

Priorities::Priorities(const string & filename)	//read brain from file
:numSpecialFeatures(8)
{
	ifstream infile(filename.c_str());

	// object_num [list of features]
	// brain data or "R" for random brain
	objectNetworks.resize(255);
	buildingNetworks.resize(9);

	char type;
	int object_num;
	int town_num;
	int building_num;
	while(infile>>type)
	{
		switch(type)
		{
		case 'o'://map object
			infile >> object_num;
			objectNetworks[object_num].push_back(Network(infile));
			break;
		case 't'://town building
			infile >> town_num >> building_num;
			buildingNetworks[town_num][building_num]=Network(infile);

			break;
		}
	}
}

void Priorities::fillFeatures(const CGeniusAI::HypotheticalGameState & hgs)
{
	stateFeatures.clear();
	stateFeatures.resize(50);
	for(int i = 0; i < stateFeatures.size(); i++)
		stateFeatures[i]=0;
	for(int i = 0; i < hgs.resourceAmounts.size();i++)	//features 0-7 are resources
		stateFeatures[i]=hgs.resourceAmounts[i];

	//TODO:												//features 8-15 are incomes

	specialFeaturesStart = 16;							//features 16-23 are special features (filled in by get functions before ANN)
	
	stateFeatures[24] = hgs.AI->m_cb->getDate();
	stateFeatures[25] = 1;
	
	
}

float Priorities::getCost(vector<int> &resourceCosts,const CGHeroInstance * moved,int distOutOfTheWay)
{
	if(resourceCosts.size()==0)return -1;
	//TODO: replace with ann
	float cost = resourceCosts[0]/4.0+resourceCosts[1]/2.0+resourceCosts[2]/4.0+resourceCosts[3]/2.0+resourceCosts[4]/2.0+resourceCosts[5]/2.0+resourceCosts[6]/3000.0;
	
	if(moved!=NULL)						//TODO: multiply by importance of hero
		cost+=distOutOfTheWay/10000.0;
	return cost;
}

float Priorities::getValue(const CGeniusAI::AIObjective & obj)
{	//resource
	
	vector<int> resourceAmounts(8,0);
	int amount;

	if(obj.type==CGeniusAI::AIObjective::finishTurn)	//TODO: replace with value of visiting that object divided by days till completed
		return .0001;			//small nonzero
	float a;
	if(obj.type==CGeniusAI::AIObjective::attack)
		return 100;
	if(dynamic_cast<const CGeniusAI::HeroObjective* >(&obj))
	{
		const CGeniusAI::HeroObjective* hobj = dynamic_cast<const CGeniusAI::HeroObjective* >(&obj);
		stateFeatures[16] = hobj->whoCanAchieve.front()->h->getTotalStrength();
		if(dynamic_cast<const CArmedInstance*>(hobj->object))
			stateFeatures[17] = dynamic_cast<const CArmedInstance*>(hobj->object)->getArmyStrength();
		switch(hobj->object->ID)
		{
		case 5: //artifact //TODO: return value of each artifact
			return 0;
		case 79://resources on the ground
			switch(hobj->object->subID)
			{
			case 6:
				amount = 800;
				break;
			case 0: case 2:
				amount = 9.5;	//will be rounded, sad
				break;
			default:
				amount = 4;
				break;
			}
			resourceAmounts[hobj->object->subID]=amount;
			return getCost(resourceAmounts,NULL,0);
			break;
		case 55://mystical garden
			resourceAmounts[6]=500;
			a=getCost(resourceAmounts,NULL,0);
			resourceAmounts[6]=0;
			resourceAmounts[5]=5;
			return (a+getCost(resourceAmounts,NULL,0))*.5;
		case 109:
			resourceAmounts[6]=1000;
			return getCost(resourceAmounts,NULL,0);
		case 12://campfire
			resourceAmounts[6]=500;
			for(int i = 0; i < 6;i++)
				resourceAmounts[i]=1;
			return getCost(resourceAmounts,NULL,0);
		case 112://windmill
			for(int i = 1; i < 6;i++)//no wood
				resourceAmounts[i]=1;
			return getCost(resourceAmounts,NULL,0);
			break;
		case 49://magic well
			//TODO: add features for hero's spell points
			break;
		case 34: //hero
			if(dynamic_cast<const CGHeroInstance*>(hobj->object)->getOwner()==obj.AI->m_cb->getMyColor())//friendly hero
			{
				
				stateFeatures[17] = dynamic_cast<const CGHeroInstance*>(hobj->object)->getTotalStrength();
				return objectNetworks[34][0].feedForward(stateFeatures);
			}
			else
			{
				stateFeatures[17] = dynamic_cast<const CGHeroInstance*>(hobj->object)->getTotalStrength();
				return objectNetworks[34][1].feedForward(stateFeatures);
			}

			break;
		case 98:
			if(dynamic_cast<const CGTownInstance*>(hobj->object)->getOwner()==obj.AI->m_cb->getMyColor())//friendly town
			{
				stateFeatures[17] = dynamic_cast<const CGTownInstance*>(hobj->object)->getArmyStrength();
				return 0; // objectNetworks[98][0].feedForward(stateFeatures);
			}
			else
			{
				stateFeatures[17] = dynamic_cast<const CGTownInstance*>(hobj->object)->getArmyStrength();
				return 0; //objectNetworks[98][1].feedForward(stateFeatures);
			}

			break;
		case 88:
			//TODO: average value of unknown level 1 spell, or value of known spell
		case 89:
			//TODO: average value of unknown level 2 spell, or value of known spell
		case 90:
			//TODO: average value of unknown level 3 spell, or value of known spell
			return 0;
		break;
		case 215://quest guard
			return 0;
		case 53:	//various mines
			return 0; //out of range crash
			//objectNetworks[53][hobj->object->subID].feedForward(stateFeatures);
		case 113://TODO: replace with value of skill for the hero
			return 0;
		case 103:case 58://TODO: replace with value of seeing x number of new tiles 
			return 0;
		default:
			if(objectNetworks[hobj->object->ID].size()!=0)
				return objectNetworks[hobj->object->ID][0].feedForward(stateFeatures);
			cout << "don't know the value of ";
			switch(obj.type)
			{
			case CGeniusAI::AIObjective::visit:
				cout << "visiting " << hobj->object->ID;
				break;
			case CGeniusAI::AIObjective::attack:
				cout << "attacking " << hobj->object->ID;
				break;
			case CGeniusAI::AIObjective::finishTurn:
				obj.print();
				break;
			}
			cout << endl;
		}
	}
	else	//town objective
	{
		if(obj.type == CGeniusAI::AIObjective::buildBuilding)
		{
			const CGeniusAI::TownObjective* tnObj = dynamic_cast<const CGeniusAI::TownObjective* >(&obj);
			if(buildingNetworks[tnObj->whichTown->t->subID].find(tnObj->which)!=buildingNetworks[tnObj->whichTown->t->subID].end())
				return buildingNetworks[tnObj->whichTown->t->subID][tnObj->which].feedForward(stateFeatures);
			else
			{
				cout << "don't know the value of ";
				obj.print();
				cout << endl;
			}

		}
	}


	return 0;
}
#endif