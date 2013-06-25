#pragma once
#include "..\..\lib\CGameInterface.h"


class ObjectVisitingModule : public CAutomationModule
{
	std::vector<const CGObjectInstance *> destinations;
	std::set<const CGObjectInstance *> visitedThisWeek;
	int week;

public:
	ObjectVisitingModule(void);
	~ObjectVisitingModule(void);

	virtual void receivedMessage(const boost::any &msg);
	virtual void executeInternal();
	virtual void heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start);

	bool isInterestingObject(const CGObjectInstance *obj) const;
	std::vector<const CGObjectInstance *> getDestinations() const;
	void printObjects(const std::vector<const CGObjectInstance *> &objs)const;
	const CGHeroInstance *myHero() const;

	bool moveHero(const CGHeroInstance *hero, int3 dst);
};

