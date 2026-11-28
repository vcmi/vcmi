/*
 * ModificatorScheduler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Zone.h"
#include <functional>

namespace rmg
{
/// Runs each job once, after the jobs it depends on. Throws rmgException when a job fails.
void runModificators(TModificators jobs, bool singleThread, const std::function<void(const Modificator &)> & onFinished);
}
