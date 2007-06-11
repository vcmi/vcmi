#ifndef COBJECTHANDLER_H
#define COBJECTHANDLER_H

#include <string>
#include <vector>

class CSpecObjInfo //class with object - specific info (eg. different information for creatures and heroes); use inheritance to make object - specific classes
{
};

class CObject //typical object that can be encountered on a map
{
public:
	std::string name; //object's name
};

class CObjectInstance //instance of object
{
public:
	int defNumber; //specifies number of def file with animation of this object
	int id; //number of object in CObjectHandler's vector
	int x, y, z; // position
	CSpecObjInfo * info; //pointer to something with additional information
};

class CObjectHandler
{
public:
	std::vector<CObject> objects; //vector of objects; i-th object in vector has subnumber i
	std::vector<CObjectInstance> objInstances; //vector with objects on map
	void loadObjects();
};


#endif //COBJECTHANDLER_H