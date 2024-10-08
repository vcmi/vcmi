/*
 * convpathqstring.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

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
