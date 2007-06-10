#include "CSemiDefHandler.h"
class CSemiLodHandler
{
public:
	std::string ourName; // name of our lod
	void openLod(std::string path);
	CSemiDefHandler * giveDef(std::string name); //loads def from our lod
};