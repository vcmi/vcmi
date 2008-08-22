#pragma  once
#include <boost/function.hpp>

template<typename Signature>
class CFunctionList
{
public:
	std::vector<boost::function<Signature> > funcs;

	CFunctionList(int){};
	CFunctionList(){};
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
	const boost::function<Signature> & operator=(const boost::function<Signature> &first)
	{
		funcs.push_back(first);
		return first;
	}
	operator bool() const
	{
		return funcs.size();
	}
	void operator()() const
	{
		std::vector<boost::function<Signature> > funcs2 = funcs; //backup
		for(int i=0;i<funcs2.size(); i++)
			funcs2[i]();
	}
	template <typename Arg> 
	void operator()(const Arg & a) const
	{
		std::vector<boost::function<Signature> > funcs2 = funcs; //backup
		for(int i=0;i<funcs2.size(); i++)
			funcs2[i](a);
	}
};