/*
 * EntityIdentifiers.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "MetaIdentifier.h"

VCMI_LIB_NAMESPACE_BEGIN

const MetaIdentifier MetaIdentifier::NONE("", "", -1);

MetaIdentifier::MetaIdentifier():
	integerForm(-1)
{}

MetaIdentifier::MetaIdentifier(const std::string & entityType, const std::string & identifier)
	: stringForm(identifier)
	, integerForm(-1)
{
	onDeserialized();
}

MetaIdentifier::MetaIdentifier(const std::string & entityType, const std::string & identifier, int32_t value)
	: stringForm(identifier)
	, integerForm(value)
{
}

bool MetaIdentifier::operator == (const MetaIdentifier & other) const
{
	assert( (stringForm == other.stringForm) ? (integerForm == other.integerForm) : true );

	return stringForm == other.stringForm;
}

bool MetaIdentifier::operator != (const MetaIdentifier & other) const
{
	return !(*this == other);
}

bool MetaIdentifier::operator < (const MetaIdentifier & other) const
{
	assert(0);
}

int32_t MetaIdentifier::getNum() const
{
	return integerForm;
}

std::string MetaIdentifier::toString() const
{
	return stringForm;
}

void MetaIdentifier::onDeserialized()
{
	assert(0); //TODO
}

VCMI_LIB_NAMESPACE_END
