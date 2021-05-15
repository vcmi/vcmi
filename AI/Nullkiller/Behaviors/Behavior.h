#pragma once

#include "../VCAI.h"

class Behavior
{
public:
	virtual Goals::TGoalVec getTasks() = 0;
	virtual std::string toString() const = 0;
};