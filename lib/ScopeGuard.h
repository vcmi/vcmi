/*
 * ScopeGuard.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN


namespace vstd
{
	template<typename Func>
	class ScopeGuard
	{
		bool fire;
		Func f;

		explicit ScopeGuard(ScopeGuard&);
		ScopeGuard& operator=(ScopeGuard&);
	public:
		ScopeGuard(ScopeGuard &&other):
		    fire(false),
			f(other.f)
		{
			std::swap(fire, other.fire);
		}

		explicit ScopeGuard(Func && f):
			fire(true),
			f(std::forward<Func>(f))
		{}
		~ScopeGuard()
		{
			if(fire)
				f();
		}
	};

	template <typename Func>
	ScopeGuard<Func> makeScopeGuard(Func&& exitScope)
	{
		return ScopeGuard<Func>(std::forward<Func>(exitScope));
	}
}

VCMI_LIB_NAMESPACE_END
