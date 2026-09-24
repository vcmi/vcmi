/*
 * ActivityID.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

/// Server-side identity of an Activity. Never leaves the server: an activity may put
/// no questions to a player at all, or several in turn, so what a player is asked
/// carries a QuestionID of its own instead.
class ActivityID : public StaticIdentifier<ActivityID>
{
public:
	using StaticIdentifier<ActivityID>::StaticIdentifier;
};
