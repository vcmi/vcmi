/*
 * ResourceConverter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
 
 // Splits def files that are shared between factions and converts pcx to bmp
void convertExtractedResourceFiles(bool splitDefs, bool convertPcxToPng, bool deleteOriginals);
