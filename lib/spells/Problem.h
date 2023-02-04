/*
 * Problem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Magic.h>

#include "../NetPacksBase.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace detail
{

class DLL_LINKAGE ProblemImpl : public Problem
{
public:
	ProblemImpl() = default;
	virtual ~ProblemImpl() = default;

	void add(MetaString && description, Severity severity = CRITICAL) override;

	void getAll(std::vector<std::string> & target) const override;
//	void getMostSevere(std::vector<std::string> & target) const;
//	std::string getMostSevere() const;
private:
	std::vector<std::pair<MetaString, Severity>> data;
};

}
}

VCMI_LIB_NAMESPACE_END
