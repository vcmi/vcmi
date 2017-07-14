/*
 * LogicalExpression.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LogicalExpression.h"

#include "VCMI_Lib.h"
#include "CGeneralTextHandler.h"

std::string LogicalExpressionDetail::getTextForOperator(std::string operation)
{
	//placed in cpp mostly to avoid unnecessary includes in header
	return VLC->generaltexth->localizedTexts["logicalExpressions"][operation].String();
}
