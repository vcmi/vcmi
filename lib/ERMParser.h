#pragma once
#include "../global.h"

class ERMParser
{
private:
	std::string srcFile;
	int parsedLine;

	void parseLine(std::string line);

public:
	ERMParser(std::string file);
	void parseFile();
};
