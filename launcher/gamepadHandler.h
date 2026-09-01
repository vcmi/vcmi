/*
 * gamepadHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QObject>

/// Polls connected game controllers so that the game can be started without mouse or keyboard.
/// Android is handled in ActivityLauncher.java instead - SDL can't deliver input to a Qt activity.
class GamepadHandler : public QObject
{
	Q_OBJECT

public:
	explicit GamepadHandler(QObject * parent = nullptr);
	~GamepadHandler() override;

signals:
	void startGameRequested();

private:
	void poll();
};
