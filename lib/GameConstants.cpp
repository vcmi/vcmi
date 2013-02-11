/*
 * GameConstants.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "VCMI_Lib.h"
#include "CDefObjInfoHandler.h"
#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CSpellHandler.h"

#define ID_LIKE_OPERATORS_INTERNAL(A, B, AN, BN)	\
bool operator==(const A & a, const B & b)			\
{													\
	return AN == BN ;								\
}													\
bool operator!=(const A & a, const B & b)			\
{													\
	return AN != BN ;								\
}													\
bool operator<(const A & a, const B & b)			\
{													\
	return AN < BN ;								\
}													\
bool operator<=(const A & a, const B & b)			\
{													\
	return AN <= BN ;								\
}													\
bool operator>(const A & a, const B & b)			\
{													\
	return AN > BN ;								\
}													\
bool operator>=(const A & a, const B & b)			\
{													\
	return AN >= BN ;								\
}

#define ID_LIKE_OPERATORS(CLASS_NAME, ENUM_NAME)	\
	ID_LIKE_OPERATORS_INTERNAL(CLASS_NAME, CLASS_NAME, a.num, b.num)	\
	ID_LIKE_OPERATORS_INTERNAL(CLASS_NAME, ENUM_NAME, a.num, b)	\
	ID_LIKE_OPERATORS_INTERNAL(ENUM_NAME, CLASS_NAME, a, b.num)


ID_LIKE_OPERATORS(Obj, Obj::EObj)

ID_LIKE_OPERATORS(ArtifactID, ArtifactID::EArtifactID)

ID_LIKE_OPERATORS(CreatureID, CreatureID::ECreatureID)

ID_LIKE_OPERATORS(SpellID, SpellID::ESpellID)

ID_LIKE_OPERATORS(BuildingID, BuildingID::EBuildingID)


bmap<int, ConstTransitivePtr<CGDefInfo> > & Obj::toDefObjInfo() const
{
	return VLC->dobjinfo->gobjs[*this];
}

CArtifact * ArtifactID::toArtifact() const
{
	return VLC->arth->artifacts[*this];
}

CCreature * CreatureID::toCreature() const
{
	return VLC->creh->creatures[*this];
}

CSpell * SpellID::toSpell() const
{
	return VLC->spellh->spells[*this];
}
