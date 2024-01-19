/*
* PrisonHeroPlacer, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once
#include "../Zone.h"
#include "../Functions.h"
#include "../../mapObjects/ObjectTemplate.h"

VCMI_LIB_NAMESPACE_BEGIN

class CRandomGenerator;

class PrisonHeroPlacer : public Modificator
{
public:
	MODIFICATOR(PrisonHeroPlacer);

	void process() override;
	void init() override;

	int getPrisonsRemaning() const;
	[[nodiscard]] HeroTypeID drawRandomHero();
	void restoreDrawnHero(const HeroTypeID & hid);

private:
    void getAllowedHeroes();
	size_t reservedHeroes;

protected:

    std::vector<HeroTypeID> allowedHeroes;
};

VCMI_LIB_NAMESPACE_END
