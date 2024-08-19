/*
 * AssetGenerator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN


class DLL_LINKAGE AssetGenerator
{
    static void createBigSpellBook(std::string filename);
    static void createAdventureOptionsCleanBackground(std::string filename);

public:
    static void generate();
};

VCMI_LIB_NAMESPACE_END
