#ifndef CSNDHANDLER_H
#define CSNDHANDLER_H

#include <vector>
#include <fstream>
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
	~CSndHandler();
	CSndHandler(std::string fname);
	void extract(std::string srcfile, std::string dstfile, bool caseSens=true); //saves selected file
	unsigned char * extract (std::string srcfile, int & size); //return selecte file
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
	void extract(std::string srcfile, std::string dstfile, bool caseSens=true); //saves selected file
	unsigned char * extract (std::string srcfile, int & size); //return selecte file
	void extract(int index, std::string dstfile); //saves selected file
	MemberFile getFile(std::string name); //nie testowane - sprawdzic
};


#endif //CSNDHANDLER_H