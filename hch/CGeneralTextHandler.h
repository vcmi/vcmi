#ifndef __CGENERALTEXTHANDLER_H__
#define __CGENERALTEXTHANDLER_H__
#include "../global.h"
#include <string>
#include <vector>
DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
std::string readTo(std::string &in, int &it, char end);
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

	//artifacts
	std::vector<std::string> artifEvents;
	std::vector<std::string> artifNames;
	std::vector<std::string> artifDescriptions;

	//towns
	std::vector<std::string> tcommands, hcommands; //texts for town screen and town hall screen
	std::vector<std::vector<std::string> > townNames; //[type id] => vec of names of instances
	std::vector<std::string> townTypes; //castle, rampart, tower, etc
	std::map<int, std::map<int, std::pair<std::string, std::string> > > buildings; //map[town id][building id] => pair<name, description>

	std::vector<std::pair<std::string,std::string> > zelp;
	std::string lossCondtions[4];
	std::string victoryConditions[14];

	//objects
	std::vector<std::string> names; //vector of objects; i-th object in vector has subnumber i
	std::vector<std::string> creGens; //names of creatures' generators
	std::vector<std::string> advobtxt;
	std::vector<std::string> xtrainfo;
	std::vector<std::string> restypes;
	std::vector<std::pair<std::string,std::string> > mines; //first - name; second - event description


	std::string getTitle(std::string text);
	std::string getDescr(std::string text);

	void loadTexts();
	void load();
};



#endif // __CGENERALTEXTHANDLER_H__
