#pragma once
#include "../global.h"

/// Struct which stores name, date and a value which says if the file is located in LOD
struct FileInfo
{
	std::string name; // file name with full path and extension
	std::time_t date;
	bool inLod; //tells if this file is located in Lod
};

class DLL_EXPORT CFileUtility
{
public:
	CFileUtility(void);
	~CFileUtility(void);

	static void getFilesWithExt(std::vector<FileInfo> &out, const std::string &dirname, const std::string &ext);
};

