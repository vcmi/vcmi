/*
 * ReplaySelectionWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

/// Dialogs that let the player pick what shall be replayed
namespace ReplaySelection
{
	/// Lists the turns that are still in memory plus every full game recording found on disk
	void showSelectionDialog();
}
