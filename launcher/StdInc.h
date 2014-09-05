#pragma once

#include "../Global.h"

#include <QtWidgets>
#include <QStringList>
#include <QSet>
#include <QVector>
#include <QList>
#include <QString>
#include <QFile>

inline QString pathToQString(const boost::filesystem::path & path)
{
#ifdef VCMI_WINDOWS
	return QString::fromStdWString(path.wstring());
#else
	return QString::fromStdString(path.string());
#endif
}