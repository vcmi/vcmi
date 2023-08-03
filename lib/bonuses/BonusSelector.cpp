/*
 * BonusSelector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BonusSelector.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace Selector
{
	DLL_LINKAGE CSelectFieldEqual<BonusType> & type()
	{
		static CSelectFieldEqual<BonusType> stype(&Bonus::type);
		return stype;
	}

	DLL_LINKAGE CSelectFieldEqual<TBonusSubtype> & subtype()
	{
		static CSelectFieldEqual<TBonusSubtype> ssubtype(&Bonus::subtype);
		return ssubtype;
	}

	DLL_LINKAGE CSelectFieldEqual<CAddInfo> & info()
	{
		static CSelectFieldEqual<CAddInfo> sinfo(&Bonus::additionalInfo);
		return sinfo;
	}

	DLL_LINKAGE CSelectFieldEqual<BonusSource> & sourceType()
	{
		static CSelectFieldEqual<BonusSource> ssourceType(&Bonus::source);
		return ssourceType;
	}

	DLL_LINKAGE CSelectFieldEqual<BonusSource> & targetSourceType()
	{
		static CSelectFieldEqual<BonusSource> ssourceType(&Bonus::targetSourceType);
		return ssourceType;
	}

	DLL_LINKAGE CSelectFieldEqual<BonusLimitEffect> & effectRange()
	{
		static CSelectFieldEqual<BonusLimitEffect> seffectRange(&Bonus::effectRange);
		return seffectRange;
	}

	DLL_LINKAGE CWillLastTurns turns;
	DLL_LINKAGE CWillLastDays days;

	CSelector DLL_LINKAGE typeSubtype(BonusType Type, TBonusSubtype Subtype)
	{
		return type()(Type).And(subtype()(Subtype));
	}

	CSelector DLL_LINKAGE typeSubtypeInfo(BonusType type, TBonusSubtype subtype, const CAddInfo & info)
	{
		return CSelectFieldEqual<BonusType>(&Bonus::type)(type)
			.And(CSelectFieldEqual<TBonusSubtype>(&Bonus::subtype)(subtype))
			.And(CSelectFieldEqual<CAddInfo>(&Bonus::additionalInfo)(info));
	}

	CSelector DLL_LINKAGE source(BonusSource source, ui32 sourceID)
	{
		return CSelectFieldEqual<BonusSource>(&Bonus::source)(source)
			.And(CSelectFieldEqual<ui32>(&Bonus::sid)(sourceID));
	}

	CSelector DLL_LINKAGE sourceTypeSel(BonusSource source)
	{
		return CSelectFieldEqual<BonusSource>(&Bonus::source)(source);
	}

	CSelector DLL_LINKAGE valueType(BonusValueType valType)
	{
		return CSelectFieldEqual<BonusValueType>(&Bonus::valType)(valType);
	}

	DLL_LINKAGE CSelector all([](const Bonus * b){return true;});
	DLL_LINKAGE CSelector none([](const Bonus * b){return false;});
}

VCMI_LIB_NAMESPACE_END