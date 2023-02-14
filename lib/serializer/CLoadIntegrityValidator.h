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

VCMI_LIB_NAMESPACE_BEGIN

/// Simple byte-to-byte saves comparator
class DLL_LINKAGE CLoadIntegrityValidator
	: public IBinaryReader
{
public:
	BinaryDeserializer serializer;
	std::unique_ptr<CLoadFile> primaryFile, controlFile;
	bool foundDesync;

	CLoadIntegrityValidator(const boost::filesystem::path &primaryFileName, const boost::filesystem::path &controlFileName, int minimalVersion = SERIALIZATION_VERSION); //throws!

	int read( void * data, unsigned size) override; //throws!
	void checkMagicBytes(const std::string & text) const;

	std::unique_ptr<CLoadFile> decay(); //returns primary file. CLoadIntegrityValidator stops being usable anymore
};

VCMI_LIB_NAMESPACE_END
