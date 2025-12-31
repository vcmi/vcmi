/*
 * render.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BAI/v13/action.h"
#include "BAI/v13/state.h"
#include "schema/base.h"

namespace MMAI::BAI::V13
{
std::string Render(const Schema::IState * istate, const Action * action);
void Verify(const State * state);
}
