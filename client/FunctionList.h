#pragma  once
#include <boost/function.hpp>

template<typename Signature, typename Allocator = std::allocator<void> >
class CFunctionList
{
public:
	std::vector<boost::function<Signature,Allocator> > funcs;

	CFunctionList(int){};
	CFunctionList(){};
	CFunctionList(const boost::function<Signature,Allocator> &first)
	{
		funcs.push_back(first);
	}
	CFunctionList & operator+=(const boost::function<Signature,Allocator> &first)
	{
		funcs.push_back(first);
		return *this;
	}
	const boost::function<Signature,Allocator> & operator=(const boost::function<Signature,Allocator> &first)
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
		std::vector<boost::function<Signature,Allocator> > funcs2 = funcs; //backup
		for(int i=0;i<funcs2.size(); i++)
			funcs2[i]();
	}
};