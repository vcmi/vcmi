//
//  ObstaclesPlacer.hpp
//  vcmi
//
//  Created by nordsoft on 06.07.2022.
//

#pragma once
#include "Zone.h"

class ObstaclePlacer: public Modificator
{
public:
	MODIFICATOR(ObstaclePlacer);
	
	void process() override;
	void init() override;
};
