//
//  RockPlacer.hpp
//  vcmi
//
//  Created by nordsoft on 06.07.2022.
//

#pragma once
#include "Zone.h"

class RockPlacer: public Modificator
{
public:
	MODIFICATOR(RockPlacer);
	
	void process() override;
	void init() override;
};
