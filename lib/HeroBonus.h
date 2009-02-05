#pragma once
#include "../global.h"
#include <string>

struct DLL_EXPORT HeroBonus
{
	enum BonusType{NONE, MOVEMENT, LAND_MOVEMENT, SEA_MOVEMENT, MORALE, LUCK, MORALE_AND_LUCK};
	enum BonusDuration{PERMANENT, ONE_BATTLE, ONE_DAY, ONE_WEEK};
	enum BonusSource{ARTIFACT, OBJECT};

	ui8 duration; //uses BonusDuration values
	ui8 type; //uses BonusType values - says to what is this bonus
	ui8 source;//uses BonusSource values - what gave that bonus
	si32 val;//for morale/luck [-3,+3], others any
	ui32 id; //id of object/artifact
	std::string description; 

	HeroBonus(ui8 Dur, ui8 Type, ui8 Src, si32 Val, ui32 ID, std::string Desc)
		:duration(Dur), type(Type), source(Src), val(Val), id(ID), description(Desc)
	{}
	HeroBonus(){};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & duration & type & source & val & id & description;
	}
};