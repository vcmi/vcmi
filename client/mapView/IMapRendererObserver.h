/*
 * IMapRendererObserver.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class int3;
class CGObjectInstance;
class CGHeroInstance;

VCMI_LIB_NAMESPACE_END

class IMapObjectObserver
{
public:
	IMapObjectObserver();
	virtual ~IMapObjectObserver();

	virtual bool hasOngoingAnimations() = 0;

	/// Plays fade-in animation and adds object to map
	virtual void onObjectFadeIn(const CGObjectInstance * obj) = 0;

	/// Plays fade-out animation and removed object from map
	virtual void onObjectFadeOut(const CGObjectInstance * obj) = 0;

	/// Adds object to map instantly, with no animation
	virtual void onObjectInstantAdd(const CGObjectInstance * obj) = 0;

	/// Removes object from map instantly, with no animation
	virtual void onObjectInstantRemove(const CGObjectInstance * obj) = 0;

	/// Perform hero movement animation, moving hero across terrain
	virtual void onHeroMoved(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;

	/// Perform initialization of hero teleportation animation with terrain fade animation
	virtual void onBeforeHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;
	virtual void onAfterHeroTeleported(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;

	virtual void onBeforeHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;
	virtual void onAfterHeroEmbark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;

	virtual void onBeforeHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;
	virtual void onAfterHeroDisembark(const CGHeroInstance * obj, const int3 & from, const int3 & dest) = 0;
};
