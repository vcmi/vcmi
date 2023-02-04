/*
 * Problem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Problem.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace detail
{

void ProblemImpl::add(MetaString && description, Severity severity)
{
	data.emplace_back(description, severity);
}

void ProblemImpl::getAll(std::vector<std::string> & target) const
{
	for(const auto & p : data)
		target.push_back(p.first.toString());
}

//void ProblemImpl::getMostSevere(std::vector<std::string> & target) const
//{
//	//TODO:
//}
//
//std::string ProblemImpl::getMostSevere() const
//{
//	std::vector<std::string> temp;
//	getMostSevere(temp);
//	return temp.empty() ? "" : temp.front();
//}


}
}

VCMI_LIB_NAMESPACE_END
