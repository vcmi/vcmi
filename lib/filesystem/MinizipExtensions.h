#pragma once

/*
 * MinizipExtensions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#ifdef USE_SYSTEM_MINIZIP
#include <minizip/unzip.h>
#include <minizip/zip.h>
#include <minizip/ioapi.h>
#else
#include "../minizip/unzip.h"
#include "../minizip/zip.h"
#include "../minizip/ioapi.h"
#endif

#include "CInputOutputStream.h"

class CIOApi
{
public:
	virtual ~CIOApi(){};
	
	void fillApiStructure(zlib_filefunc64_def & api);
	
	virtual CInputOutputStream * openFile(const std::string & filename, int mode) = 0;	

};
