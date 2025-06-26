/*
 * StdInc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../Global.h"

#include <QtWidgets>
#ifdef ENABLE_TEMPLATE_EDITOR
#include <QtSvg>
#include <QSvgRenderer>
#include <QDomDocument>
#endif
#include <QStringList>
#include <QSet>
#include <QVector>
#include <QList>
#include <QString>
#include <QFile>

#include "../vcmiqt/convpathqstring.h"

VCMI_LIB_USING_NAMESPACE

using NumericPointer = typename std::conditional_t<sizeof(void *) == sizeof(unsigned long long),
												 unsigned long long, unsigned int>;

template<class Type>
NumericPointer data_cast(Type * _pointer)
{
	static_assert(sizeof(Type *) == sizeof(NumericPointer),
				  "Cannot compile for that architecture, see NumericPointer definition");

	return reinterpret_cast<NumericPointer>(_pointer);
}

template<class Type>
Type * data_cast(NumericPointer _numeric)
{
	static_assert(sizeof(Type *) == sizeof(NumericPointer),
				  "Cannot compile for that architecture, see NumericPointer definition");

	return reinterpret_cast<Type *>(_numeric);
}
