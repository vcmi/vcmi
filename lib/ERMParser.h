#pragma once
#include "../global.h"

class ERMParser
{
private:
	std::string srcFile;


	void parseLine(std::string line);

public:
	ERMParser(std::string file);
	void parseFile();
};
