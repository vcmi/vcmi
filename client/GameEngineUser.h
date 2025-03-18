/*
 * GameEngineUser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class IGameEngineUser
{
public:
	~IGameEngineUser() = default;

	/// Called when user presses hotkey that activates global lobby
	virtual void onGlobalLobbyInterfaceActivated() = 0;

	/// Called on every game tick for game to update its state
	virtual void onUpdate() = 0;

	/// Called when app shutdown has been requested in any way - exit button, Alt-F4, etc
	virtual void onShutdownRequested(bool askForConfirmation) = 0;

	/// Returns true if all input events should be captured and ignored
	virtual bool capturedAllEvents() = 0;
};
