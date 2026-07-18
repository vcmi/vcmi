/*
 * WidgetInitializationException.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QString>

class WidgetInitializationException : public std::exception
{
	const QString msg;

public:
	explicit WidgetInitializationException(const QString & message) : msg(message) {}

	QString message() const
	{
		return msg;
	}
};
