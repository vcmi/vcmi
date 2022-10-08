#pragma once

#include "../Global.h"

#define VCMI_EDITOR_VERSION "0.1"
#define VCMI_EDITOR_NAME "VCMI Map Editor"

#include <QtWidgets>
#include <QStringList>
#include <QSet>
#include <QVector>
#include <QList>
#include <QString>
#include <QFile>

using NumericPointer = unsigned long long;

template<class Type>
NumericPointer data_cast(Type * _pointer)
{
	static_assert(sizeof(Type *) == sizeof(NumericPointer),
				  "Compiled for 64 bit arcitecture. Use NumericPointer = unsigned int");

	return reinterpret_cast<NumericPointer>(_pointer);
}

template<class Type>
Type * data_cast(NumericPointer _numeric)
{
	static_assert(sizeof(Type *) == sizeof(NumericPointer),
				  "Compiled for 64 bit arcitecture. Use NumericPointer = unsigned int");

	return reinterpret_cast<Type *>(_numeric);
}

inline QString pathToQString(const boost::filesystem::path & path)
{
#ifdef VCMI_WINDOWS
	return QString::fromStdWString(path.wstring());
#else
	return QString::fromStdString(path.string());
#endif
}

inline boost::filesystem::path qstringToPath(const QString & path)
{
#ifdef VCMI_WINDOWS
	return boost::filesystem::path(path.toStdWString());
#else
	return boost::filesystem::path(path.toUtf8().data());
#endif
}
