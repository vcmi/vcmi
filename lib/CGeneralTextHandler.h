#pragma once

/*
 * CGeneralTextHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CInputStream;

/// Parser for any text files from H3
class CLegacyConfigParser
{
	std::unique_ptr<char[]> data;
	char * curr;
	char * end;

	void init(const std::unique_ptr<CInputStream> & input);

	/// extracts part of quoted string.
	std::string extractQuotedPart();

	/// extracts quoted string. Any end of lines are ignored, double-quote is considered as "escaping"
	std::string extractQuotedString();

	/// extracts non-quoted string
	std::string extractNormalString();

public:
	/// read one entry from current line. Return ""/0 if end of line reached
	std::string readString();
	float readNumber();

	/// returns true if next entry is empty
	bool isNextEntryEmpty();

	/// end current line
	bool endLine();

	CLegacyConfigParser(std::string URI);
	CLegacyConfigParser(const std::unique_ptr<CInputStream> & input);
};

class DLL_LINKAGE CGeneralTextHandler //Handles general texts
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
	std::vector<std::string> lossCondtions;
	std::vector<std::string> victoryConditions;

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
	std::vector<std::string> tentColors;
	std::vector<std::string> threat; //power rating for neutral stacks

	//sec skills
	std::vector <std::string>  skillName;
	std::vector <std::vector <std::string> > skillInfoTexts; //[id][level] : level 0 - basic; 2 - advanced
	std::vector<std::string> levels;
	std::vector<std::string> zcrexp; //more or less useful content of that file

	//campaigns
	std::vector <std::string> campaignMapNames;
	std::vector < std::vector <std::string> > campaignRegionNames;

	std::string getTitle(const std::string & text);
	std::string getDescr(const std::string & text);

	void load();
	CGeneralTextHandler();
};
