#pragma once
#include "../global.h"
#include <fstream>

class CERMPreprocessor
{
	std::string fname;
	std::ifstream file;
	int lineNo;
	enum {INVALID, ERM, VERM} version;

	void getline(std::string &ret);

public:
	CERMPreprocessor(const std::string &Fname);
	std::string retreiveCommandLine();
};

class ERMParser
{
private:
	std::string srcFile;
	int parsedLine;
	void repairEncoding(char * str, int len) const; //removes nonstandard ascii characters from string
	void repairEncoding(std::string & str) const; //removes nonstandard ascii characters from string
	enum ELineType{COMMAND_FULL, COMMENT, UNFINISHED, END_OF};
	int countHatsBeforeSemicolon(const std::string & line) const;
	ELineType classifyLine(const std::string & line, bool inString) const;
	void parseLine(const std::string & line);


public:
	ERMParser(std::string file);
	void parseFile();
};
