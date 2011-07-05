#define VCMI_DLL
#include "ResourceSet.h"

Res::ResourceSet::ResourceSet()
{
	resize(RESOURCE_QUANTITY, 0);
}

bool Res::ResourceSet::nonZero() const
{
	for(int i = 0; i < size(); i++)
		if(at(i))
			return true;

	return false;
}

void Res::ResourceSet::amax(const TResource &val)
{
	for(int i = 0; i < size(); i++)
		::amax(at(i), val);
}

bool Res::ResourceSet::canBeAfforded(const ResourceSet &res) const
{
	return Res::canAfford(res, *this);
}

bool Res::ResourceSet::canAfford(const ResourceSet &price) const
{
	return Res::canAfford(*this, price);
}

bool Res::canAfford(const ResourceSet &res, const ResourceSet &price)
{
	assert(res.size() == price.size() && price.size() == RESOURCE_QUANTITY);
	for(int i = 0; i < RESOURCE_QUANTITY; i++)
		if(price[i] > res[i])
			return false;

	return true;
}

bool Res::ResourceSet::nziterator::valid()
{
	return cur.resType < RESOURCE_QUANTITY && cur.resVal;
}

Res::ResourceSet::nziterator Res::ResourceSet::nziterator::operator++()
{
	advance();
	return *this;
}

Res::ResourceSet::nziterator Res::ResourceSet::nziterator::operator++(int)
{
	nziterator ret = *this;
	advance();
	return ret;
}

const Res::ResourceSet::nziterator::ResEntry& Res::ResourceSet::nziterator::operator*() const
{
	return cur;
}

const Res::ResourceSet::nziterator::ResEntry * Res::ResourceSet::nziterator::operator->() const
{
	return &cur;
}

void Res::ResourceSet::nziterator::advance()
{
	do
	{
		cur.resType++;
	} while(cur.resType < RESOURCE_QUANTITY && !(cur.resVal=rs[cur.resType]));

	if(cur.resType >= RESOURCE_QUANTITY)
		cur.resVal = -1;
}

Res::ResourceSet::nziterator::nziterator(const ResourceSet &RS)
	: rs(RS)
{
	cur.resType = 0;
	cur.resVal = rs[0];

	if(!valid())
		advance();
}