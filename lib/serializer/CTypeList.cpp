/*
 * CTypeList.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CTypeList.h"

#include "../registerTypes/RegisterTypes.h"

VCMI_LIB_NAMESPACE_BEGIN

extern template void registerTypes<CTypeList>(CTypeList & s);

CTypeList typeList;

CTypeList::CTypeList()
{
	registerTypes(*this);
}

CTypeList::TypeInfoPtr CTypeList::registerType(const std::type_info *type)
{
	if(auto typeDescr = getTypeDescriptor(type, false))
		return typeDescr;  //type found, return ptr to structure

	//type not found - add it to the list and return given ID
	auto newType = std::make_shared<TypeDescriptor>();
	newType->typeID = static_cast<ui16>(typeInfos.size() + 1);
	newType->name = type->name();
	typeInfos[type] = newType;

	return newType;
}

ui16 CTypeList::getTypeID(const std::type_info *type, bool throws) const
{
	auto descriptor = getTypeDescriptor(type, throws);
	if (descriptor == nullptr)
	{
		return 0;
	}
	return descriptor->typeID;
}

CTypeList::TypeInfoPtr CTypeList::getTypeDescriptor(ui16 typeID) const
{
	auto found = std::find_if(typeInfos.begin(), typeInfos.end(), [typeID](const std::pair<const std::type_info *, TypeInfoPtr> & p) -> bool
		{
			return p.second->typeID == typeID;
		});

	if(found != typeInfos.end())
	{
		return found->second;
	}

	return {};
}

std::vector<CTypeList::TypeInfoPtr> CTypeList::castSequence(TypeInfoPtr from, TypeInfoPtr to) const
{
	if(!strcmp(from->name, to->name))
		return {};

	// Perform a simple BFS in the class hierarchy.

	auto BFS = [&](bool upcast)
	{
		std::map<TypeInfoPtr, TypeInfoPtr> previous;
		std::queue<TypeInfoPtr> q;
		q.push(to);
		while(q.size())
		{
			auto typeNode = q.front();
			q.pop();
			for(auto & weakNode : (upcast ? typeNode->parents : typeNode->children) )
			{
				auto nodeBase = weakNode.lock();
				if(!previous.count(nodeBase))
				{
					previous[nodeBase] = typeNode;
					q.push(nodeBase);
				}
			}
		}

		std::vector<TypeInfoPtr> ret;

		if(!previous.count(from))
			return ret;

		ret.push_back(from);
		TypeInfoPtr ptr = from;
		do
		{
			ptr = previous.at(ptr);
			ret.push_back(ptr);
		} while(ptr != to);

		return ret;
	};

	// Try looking both up and down.
	auto ret = BFS(true);
	if(ret.empty())
		ret = BFS(false);

	if(ret.empty())
		THROW_FORMAT("Cannot find relation between types %s and %s. Were they (and all classes between them) properly registered?", from->name % to->name);

	return ret;
}

std::vector<CTypeList::TypeInfoPtr> CTypeList::castSequence(const std::type_info *from, const std::type_info *to) const
{
	//This additional if is needed because getTypeDescriptor might fail if type is not registered
	// (and if casting is not needed, then registereing should no  be required)
	if(!strcmp(from->name(), to->name()))
		return {};

	return castSequence(getTypeDescriptor(from), getTypeDescriptor(to));
}

CTypeList::TypeInfoPtr CTypeList::getTypeDescriptor(const std::type_info *type, bool throws) const
{
	auto i = typeInfos.find(type);
	if(i != typeInfos.end())
		return i->second; //type found, return ptr to structure

	if(!throws)
		return nullptr;

	THROW_FORMAT("Cannot find type descriptor for type %s. Was it registered?", type->name());
}

VCMI_LIB_NAMESPACE_END
