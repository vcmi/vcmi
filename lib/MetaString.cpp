/*
 * CGameState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MetaString.h"

#include "CArtHandler.h"
#include "CCreatureHandler.h"
#include "CCreatureSet.h"
#include "CGeneralTextHandler.h"
#include "CSkillHandler.h"
#include "GameConstants.h"
#include "VCMI_Lib.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "spells/CSpellHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void MetaString::getLocalString(const std::pair<ui8, ui32> & txt, std::string & dst) const
{
	int type = txt.first;
	int ser = txt.second;

	if(type == ART_NAMES)
	{
		const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
		if(art)
			dst = art->getNameTranslated();
		else
			dst = "#!#";
	}
	else if(type == ART_DESCR)
	{
		const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
		if(art)
			dst = art->getDescriptionTranslated();
		else
			dst = "#!#";
	}
	else if (type == ART_EVNTS)
	{
		const auto * art = ArtifactID(ser).toArtifact(VLC->artifacts());
		if(art)
			dst = art->getEventTranslated();
		else
			dst = "#!#";
	}
	else if(type == CRE_PL_NAMES)
	{
		const auto * cre = CreatureID(ser).toCreature(VLC->creatures());
		if(cre)
			dst = cre->getNamePluralTranslated();
		else
			dst = "#!#";
	}
	else if(type == CRE_SING_NAMES)
	{
		const auto * cre = CreatureID(ser).toCreature(VLC->creatures());
		if(cre)
			dst = cre->getNameSingularTranslated();
		else
			dst = "#!#";
	}
	else if(type == MINE_NAMES)
	{
		dst = VLC->generaltexth->translate("core.minename", ser);
	}
	else if(type == MINE_EVNTS)
	{
		dst = VLC->generaltexth->translate("core.mineevnt", ser);
	}
	else if(type == SPELL_NAME)
	{
		const auto * spell = SpellID(ser).toSpell(VLC->spells());
		if(spell)
			dst = spell->getNameTranslated();
		else
			dst = "#!#";
	}
	else if(type == OBJ_NAMES)
	{
		dst = VLC->objtypeh->getObjectName(ser, 0);
	}
	else if(type == SEC_SKILL_NAME)
	{
		dst = VLC->skillh->getByIndex(ser)->getNameTranslated();
	}
	else
	{
		switch(type)
		{
		case GENERAL_TXT:
			dst = VLC->generaltexth->translate("core.genrltxt", ser);
			break;
		case RES_NAMES:
			dst = VLC->generaltexth->translate("core.restypes", ser);
			break;
		case ARRAY_TXT:
			dst = VLC->generaltexth->translate("core.arraytxt", ser);
			break;
		case CREGENS:
			dst = VLC->objtypeh->getObjectName(Obj::CREATURE_GENERATOR1, ser);
			break;
		case CREGENS4:
			dst = VLC->objtypeh->getObjectName(Obj::CREATURE_GENERATOR4, ser);
			break;
		case ADVOB_TXT:
			dst = VLC->generaltexth->translate("core.advevent", ser);
			break;
		case COLOR:
			dst = VLC->generaltexth->translate("vcmi.capitalColors", ser);
			break;
		case JK_TXT:
			dst = VLC->generaltexth->translate("core.jktext", ser);
			break;
		default:
			logGlobal->error("Failed string substitution because type is %d", type);
			dst = "#@#";
			return;
		}
	}
}

DLL_LINKAGE void MetaString::toString(std::string &dst) const
{
	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
	dst.clear();

	for(const auto & elem : message)
	{//TEXACT_STRING, TLOCAL_STRING, TNUMBER, TREPLACE_ESTRING, TREPLACE_LSTRING, TREPLACE_NUMBER
		switch(elem)
		{
		case TEXACT_STRING:
			dst += exactStrings[exSt++];
			break;
		case TLOCAL_STRING:
			{
				std::string hlp;
				getLocalString(localStrings[loSt++], hlp);
				dst += hlp;
			}
			break;
		case TNUMBER:
			dst += std::to_string(numbers[nums++]);
			break;
		case TREPLACE_ESTRING:
			boost::replace_first(dst, "%s", exactStrings[exSt++]);
			break;
		case TREPLACE_LSTRING:
			{
				std::string hlp;
				getLocalString(localStrings[loSt++], hlp);
				boost::replace_first(dst, "%s", hlp);
			}
			break;
		case TREPLACE_NUMBER:
			boost::replace_first(dst, "%d", std::to_string(numbers[nums++]));
			break;
		case TREPLACE_PLUSNUMBER:
			boost::replace_first(dst, "%+d", '+' + std::to_string(numbers[nums++]));
			break;
		default:
			logGlobal->error("MetaString processing error! Received message of type %d", int(elem));
			break;
		}
	}
}

DLL_LINKAGE std::string MetaString::toString() const
{
	std::string ret;
	toString(ret);
	return ret;
}

DLL_LINKAGE std::string MetaString::buildList () const
///used to handle loot from creature bank
{

	size_t exSt = 0;
	size_t loSt = 0;
	size_t nums = 0;
	std::string lista;
	for (int i = 0; i < message.size(); ++i)
	{
		if (i > 0 && (message[i] == TEXACT_STRING || message[i] == TLOCAL_STRING))
		{
			if (exSt == exactStrings.size() - 1)
				lista += VLC->generaltexth->allTexts[141]; //" and "
			else
				lista += ", ";
		}
		switch (message[i])
		{
			case TEXACT_STRING:
				lista += exactStrings[exSt++];
				break;
			case TLOCAL_STRING:
			{
				std::string hlp;
				getLocalString (localStrings[loSt++], hlp);
				lista += hlp;
			}
				break;
			case TNUMBER:
				lista += std::to_string(numbers[nums++]);
				break;
			case TREPLACE_ESTRING:
				lista.replace (lista.find("%s"), 2, exactStrings[exSt++]);
				break;
			case TREPLACE_LSTRING:
			{
				std::string hlp;
				getLocalString (localStrings[loSt++], hlp);
				lista.replace (lista.find("%s"), 2, hlp);
			}
				break;
			case TREPLACE_NUMBER:
				lista.replace (lista.find("%d"), 2, std::to_string(numbers[nums++]));
				break;
			default:
				logGlobal->error("MetaString processing error! Received message of type %d",int(message[i]));
		}

	}
	return lista;
}

void MetaString::addCreReplacement(const CreatureID & id, TQuantity count) //adds sing or plural name;
{
	if (!count)
		addReplacement (CRE_PL_NAMES, id); //no creatures - just empty name (eg. defeat Angels)
	else if (count == 1)
		addReplacement (CRE_SING_NAMES, id);
	else
		addReplacement (CRE_PL_NAMES, id);
}

void MetaString::addReplacement(const CStackBasicDescriptor & stack)
{
	assert(stack.type); //valid type
	addCreReplacement(stack.type->getId(), stack.count);
}

VCMI_LIB_NAMESPACE_END
