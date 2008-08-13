#ifndef CABILITYHANDLER_H
#define CABILITYHANDLER_H

#include <string>
#include <vector>

class CDefHandler;

class CAbility
{
public:
	std::string name;
	std::vector <std::string> infoTexts; //0 - basic; 2 - advanced
	int idNumber;
	bool isAllowed; //true if we can use this hero's ability (map information)
};

class CAbilityHandler
{
public:
	std::vector<CAbility *> abilities;
	CDefHandler * abils32, * abils44, * abils82;
	std::vector<std::string> levels;
	void loadAbilities();
};

#endif //CABILITYHANDLER_H
