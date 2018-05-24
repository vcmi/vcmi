/*
* Goals.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "lib/VCMI_Lib.h"
#include "lib/CBuildingHandler.h"
#include "lib/CCreatureHandler.h"
#include "lib/CTownHandler.h"
#include "../AIUtility.h"

struct HeroPtr;
class VCAI;
class FuzzyHelper;
struct SectorMap;

namespace Tasks
{
	class CTask;

	typedef std::shared_ptr<CTask> TaskPtr;
	typedef std::vector<TaskPtr> TaskList;

	/// Represents a simple command like visiting tile which needs to be executed in order to achieve some Goal
	/// In general should be added to TaskList only if we are 100% sure we can do it
	/// Replaces elementar Goal
	class CTask {
	protected:
		double priority = 0;
		int3 tile;
		HeroPtr hero;

	public:
		virtual void execute() { }
		virtual bool canExecute() { return true; }
		virtual CTask* clone() const {
			return const_cast<CTask*>(this);
		}
		virtual std::string toString() {
			return "CTask";
		}
		int3 getTile() {
			return tile;
		}
		HeroPtr getHero() {
			return hero;
		}
		double getPriority() {
			return priority;
		}
		void addAncestorPriority(double ancestorPriority) {
			priority = ancestorPriority + priority / 10.0;
		}
	};

	template<typename T> class TemplateTask : public CTask {
	public:
		TemplateTask<T> * clone() const override
		{
			return new T(static_cast<T const &>(*this)); //casting enforces template instantiation
		}
	};

	TaskPtr sptr(const CTask & tmp);
}