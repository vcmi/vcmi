#ifndef CPREGAMETEXTHANDLER_H
#define CPREGAMETEXTHANDLER_H

#include <string>

class CPreGameTextHandler //handles pre - game texts
{
public:

	std::vector<std::pair<std::string,std::string> > zelp;
	std::string lossCondtions[4];
	std::string victoryConditions[14];

	std::string getTitle(std::string text);
	std::string getDescr(std::string text);

	void loadTexts();
};


#endif //CPREGAMETEXTHANDLER_H