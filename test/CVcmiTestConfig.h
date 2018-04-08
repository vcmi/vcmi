/*
 * CVcmiTestConfig.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

/// Global setup/tear down class for unit tests.
class CVcmiTestConfig : public ::testing::Environment
{
public:
	virtual void SetUp() override;
	virtual void TearDown() override;
};
