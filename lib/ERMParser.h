#pragma once
#include "../global.h"

class ERMParser
{
private:
	std::string srcFile;
	int parsedLine;
	void repairEncoding(char * str, int len) const; //removes nonstandard ascii characters from string
	void repairEncoding(std::string & str) const; //removes nonstandard ascii characters from string
	enum ELineType{COMMAND_FULL, COMMENT, UNFINISHED_STRING, END_OF_STRING};
	int countHatsBeforeSemicolon(const std::string & line) const;
	ELineType classifyLine(const std::string & line, bool inString) const;
	void parseLine(const std::string & line);


public:
	ERMParser(std::string file);
	void parseFile();
};
