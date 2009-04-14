#ifndef FUNCTIONLIST_H
#define FUNCTIONLIST_H

#ifdef _MSC_VER
#pragma  once
#endif

#include <boost/function.hpp>

template<typename Signature>
class CFunctionList
{
public:
	std::vector<boost::function<Signature> > funcs;

	CFunctionList(int){};
	CFunctionList(){};
	template <typename Functor> 
	CFunctionList(const Functor &f)
	{
		funcs.push_back(boost::function<Signature>(f));
	}
	CFunctionList(const boost::function<Signature> &first)
	{
		funcs.push_back(first);
	}
	CFunctionList(boost::function<Signature> &first)
	{
		funcs.push_back(first);
	}
	CFunctionList & operator+=(const boost::function<Signature> &first)
	{
		funcs.push_back(first);
		return *this;
	}
	void add(const CFunctionList<Signature> &first)
	{
		for (size_t i = 0; i < first.funcs.size(); i++)
		{
			funcs.push_back(first.funcs[i]);
		}
	}
	void clear()
	{
		funcs.clear();
	}
	operator bool() const
	{
		return funcs.size();
	}
	void operator()() const
	{
		std::vector<boost::function<Signature> > funcs2 = funcs; //backup
		for(size_t i=0;i<funcs2.size(); ++i) 
		{
			funcs2[i]();
		}
	}
	template <typename Arg> 
	void operator()(const Arg & a) const
	{
		std::vector<boost::function<Signature> > funcs2 = funcs; //backup
		for(int i=0;i<funcs2.size(); i++) 
		{
			funcs2[i](a);
		}
	}
};

template<typename Signature>
class CFunctionList2
{
public:
	std::vector<boost::function<Signature> > funcs;

	CFunctionList2(int){};
	CFunctionList2(){};
	template <typename Functor> 
	CFunctionList2(const Functor &f)
	{
		funcs.push_back(boost::function<Signature>(f));
	}
	CFunctionList2(const boost::function<Signature> &first)
	{
		funcs.push_back(first);
	}
	CFunctionList2(boost::function<Signature> &first)
	{
		funcs.push_back(first);
	}
	CFunctionList2 & operator+=(const boost::function<Signature> &first)
	{
		funcs.push_back(first);
		return *this;
	}
	void clear()
	{
		funcs.clear();
	}
	operator bool() const
	{
		return funcs.size();
	}
	template <typename Arg> 
	void operator()(const Arg & a) const
	{
		std::vector<boost::function<Signature> > funcs2 = funcs; //backup
		for(size_t i=0;i<funcs2.size(); ++i) 
		{
			funcs2[i](a);
		}
	}
};

#endif //FUNCTIONLISt_H