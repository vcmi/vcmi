#ifndef __CSNDHANDLER_H__
#define __CSNDHANDLER_H__

#include <vector>
#include <fstream>
#include <map>

/*
 * CSndHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct MemberFile
{
	std::ifstream * ifs;
	int length;
};
class CSndHandler
{
protected:
	const int CHUNK;
	struct Entry
	{
		std::string name;
		int size, offset;
	};
	std::ifstream file;
	int readNormalNr (int pos, int bytCon);
	bool opened;
public:
	std::vector<Entry> entries;
	std::map<std::string, int> fimap; // map of wav file and index
	~CSndHandler();
	CSndHandler(std::string fname);
	void extract(std::string srcfile, std::string dstfile, bool caseSens=true); //saves selected file
	unsigned char * extract (std::string srcfile, int & size); //return selecte file data, NULL if file doesn't exist
	void extract(int index, std::string dstfile); //saves selected file
	MemberFile getFile(std::string name);//nie testowane - sprawdzic
	unsigned char * extract (int index, int & size); //return selecte file - NIE TESTOWANE
};
class CVidHandler
{
protected:
	const int CHUNK;
	struct Entry
	{
		std::string name;
		int size, offset, something;
	};
	std::ifstream file;
	int readNormalNr (int pos, int bytCon);
	bool opened;
public:
	std::vector<Entry> entries;
	~CVidHandler();
	CVidHandler(std::string fname);
	std::ifstream & extract(std::string srcfile);
	void extract(std::string srcfile, std::string dstfile, bool caseSens=true); //saves selected file
	unsigned char * extract (std::string srcfile, int & size); //return selecte file,
	void extract(int index, std::string dstfile); //saves selected file
	MemberFile getFile(std::string name); //nie testowane - sprawdzic
};



#endif // __CSNDHANDLER_H__
