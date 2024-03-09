/*
 * TradeItem.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/VariantIdentifier.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

using TradeItemSell = VariantIdentifier<GameResID, SlotID, ArtifactInstanceID>;
using TradeItemBuy = VariantIdentifier<GameResID, PlayerColor, ArtifactID, SecondarySkill>;

VCMI_LIB_NAMESPACE_END
