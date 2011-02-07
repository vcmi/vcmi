#ifndef __CGENERALTEXTHANDLER_H__
#define __CGENERALTEXTHANDLER_H__
#include "../global.h"
#include <string>
#include <vector>

/*
 * CGeneralTextHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

DLL_EXPORT void loadToIt(std::string &dest, const std::string &src, int &iter, int mode);
std::string readTo(const std::string &in, int &it, char end);
class DLL_EXPORT CGeneralTextHandler //Handles general texts
{
public:
	class HeroTexts
	{
	public:
		std::string bonusName, shortBonus, longBonus; //for special abilities
		std::string biography; //biography, of course
	};

	std::vector<HeroTexts> hTxts;
	std::vector<std::string> allTexts;

	std::vector<std::string> arraytxt;
	std::vector<std::string> primarySkillNames;
	std::vector<std::string> jktexts;
	std::vector<std::string> heroscrn;
	std::vector<std::string> overview;//text for Kingdom Overview window
	std::vector<std::string> colors; //names of player colors ("red",...)
	std::vector<std::string> capColors; //names of player colors with first letter capitalized ("Red",...)
	std::vector<std::string> turnDurations; //turn durations for pregame (1 Minute ... Unlimited) 

	//artifacts
	std::vector<std::string> artifEvents;
	std::vector<std::string> artifNames;
	std::vector<std::string> artifDescriptions;

	//towns
	std::vector<std::string> tcommands, hcommands, fcommands; //texts for town screen, town hall screen and fort screen
	std::vector<std::string> tavernInfo;
	std::vector<std::vector<std::string> > townNames; //[type id] => vec of names of instances
	std::vector<std::string> townTypes; //castle, rampart, tower, etc
	std::map<int, std::map<int, std::pair<std::string, std::string> > > buildings; //map[town id][building id] => pair<name, description>

	std::vector<std::pair<std::string,std::string> > zelp;
	std::string lossCondtions[4];
	std::string victoryConditions[14];

	//objects
	std::vector<std::string> names; //vector of objects; i-th object in vector has subnumber i
	std::vector<std::string> creGens; //names of creatures' generators
	std::vector<std::string> creGens4; //names of multiple creatures' generators
	std::vector<std::string> advobtxt;
	std::vector<std::string> xtrainfo;
	std::vector<std::string> restypes; //names of resources
	std::vector<std::string> terrainNames;
	std::vector<std::string> randsign;
	std::vector<std::pair<std::string,std::string> > mines; //first - name; second - event description
	std::vector<std::string> seerEmpty;
	std::vector <std::vector <std::vector <std::string> > >  quests; //[quest][type][index]
	//type: quest, progress, complete, rollover, log OR time limit //index: 0-2 seer hut, 3-5 border guard
	std::vector<std::string> seerNames;
	std::vector<std::string> threat; //power rating for neutral stacks

	//sec skills
	std::vector <std::string>  skillName;
	std::vector <std::vector <std::string> > skillInfoTexts; //[id][level] : level 0 - basic; 2 - advanced
	std::vector<std::string> levels;

	//campaigns
	std::vector <std::string> campaignMapNames;
	std::vector < std::vector <std::string> > campaignRegionNames;

	std::string getTitle(const std::string & text);
	std::string getDescr(const std::string & text);

	void load();
	CGeneralTextHandler();
};



#endif // __CGENERALTEXTHANDLER_H__
