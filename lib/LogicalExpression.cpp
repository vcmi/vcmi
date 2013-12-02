#include "StdInc.h"

#include "LogicalExpression.h"

#include "VCMI_Lib.h"
#include "CGeneralTextHandler.h"

std::string LogicalExpressionDetail::getTextForOperator(std::string operation)
{
	//placed in cpp mostly to avoid unnecessary includes in header
	return VLC->generaltexth->localizedTexts["logicalExpressions"][operation].String();
}
