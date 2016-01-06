
/*
 * CLoadIntegrityValidator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BinaryDeserializer.h"

/// Simple byte-to-byte saves comparator
class DLL_LINKAGE CLoadIntegrityValidator
	: public IBinaryReader
{
public:
	BinaryDeserializer serializer;
	unique_ptr<CLoadFile> primaryFile, controlFile;
	bool foundDesync;

	CLoadIntegrityValidator(const std::string &primaryFileName, const std::string &controlFileName, int minimalVersion = version); //throws!

	int read( void * data, unsigned size) override; //throws!
	void checkMagicBytes(const std::string &text);

	unique_ptr<CLoadFile> decay(); //returns primary file. CLoadIntegrityValidator stops being usable anymore
};
