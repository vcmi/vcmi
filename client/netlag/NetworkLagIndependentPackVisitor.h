/*
 * NetworkLagIndependentPackVisitor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/networkPacks/NetPackVisitor.h"

/// Class that detects packs that do not change any game state that predicted packs may depend on,
/// and can be applied without rolling back unconfirmed predictions
class NetworkLagIndependentPackVisitor final : public ICPackVisitor
{
	bool independentValue = false;

	void visitPackageReceived(PackageReceived & pack) override
	{
		independentValue = true;
	}
	void visitPackageApplied(PackageApplied & pack) override
	{
		independentValue = true;
	}
	void visitSystemMessage(SystemMessage & pack) override
	{
		independentValue = true;
	}
	void visitTurnTimeUpdate(TurnTimeUpdate & pack) override
	{
		independentValue = true;
	}
	void visitPlayerMessageClient(PlayerMessageClient & pack) override
	{
		independentValue = true;
	}

public:
	/// Returns true if visited pack can be applied on top of unconfirmed predictions
	bool isIndependent() const
	{
		return independentValue;
	}
};
