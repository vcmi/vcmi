/*
 * FunctionList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// List of functions that share the same signature - can be used to call all of them easily
template<typename Signature>
class CFunctionList
{
	std::vector<std::function<Signature> > funcs;
public:
	CFunctionList( std::nullptr_t ) { }
	CFunctionList( int ) { }
	CFunctionList( ){ }

	template <typename Functor> 
	CFunctionList(const Functor &f)
	{
		funcs.push_back(std::function<Signature>(f));
	}

	CFunctionList(const std::function<Signature> &first)
	{
		if (first)
			funcs.push_back(first);
	}

	CFunctionList & operator+=(const CFunctionList<Signature> &first)
	{
		for( auto & fun : first.funcs)
		{
			funcs.push_back(fun);
		}
		return *this;
	}

	void clear()
	{
		funcs.clear();
	}

	operator bool() const
	{
		return !funcs.empty();
	}

	template <typename... Args>
	void operator()(Args ... args) const
	{
		std::vector<std::function<Signature> > funcs_copy = funcs;
		for( auto & fun : funcs_copy)
		{
			if (fun)
				fun(args...);
		}
	}
};

VCMI_LIB_NAMESPACE_END
