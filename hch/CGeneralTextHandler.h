#ifndef CGENERALTEXTHANDLER_H
#define CGENERALTEXTHANDLER_H
#include "../global.h"
#include <string>
#include <vector>
DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
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
	std::vector<std::string> artifEvents;

	std::vector<std::pair<std::string,std::string> > zelp;
	std::string lossCondtions[4];
	std::string victoryConditions[14];

	std::string getTitle(std::string text);
	std::string getDescr(std::string text);

	void loadTexts();
	void load();
};


#endif //CGENERALTEXTHANDLER_H
