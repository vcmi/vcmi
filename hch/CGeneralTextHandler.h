#ifndef CGENERALTEXTHANDLER_H
#define CGENERALTEXTHANDLER_H
#include "../global.h"
#include <string>
#include <vector>
DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
class CGeneralTextHandler //Handles general texts
{
public:
	std::vector<std::string> allTexts;

	std::vector<std::string> arraytxt;
	std::vector<std::string> primarySkillNames;
	std::vector<std::string> jktexts;
	std::vector<std::string> heroscrn;
	std::vector<std::string> artifEvents;

	void load();
};


#endif //CGENERALTEXTHANDLER_H
