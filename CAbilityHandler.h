#ifndef CABILITYHANDLER_H
#define CABILITYHANDLER_H

#include <string>
#include <vector>

class CAbility
{
public:
	std::string name;
	std::string basicText, basicText2;
	std::string advText, advText2;
	std::string expText, expText2;
	int idNumber;
};

class CAbilityHandler
{
public:
	std::vector<CAbility> abilities;
	void loadAbilities();
};

#endif CABILITYHANDLER_H