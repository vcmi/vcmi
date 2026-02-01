/*
 * Discord.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

VCMI_LIB_NAMESPACE_BEGIN
struct StartInfo;
class CMap;
VCMI_LIB_NAMESPACE_END

class Discord
{
    time_t startTime = time(nullptr);
    bool enabled;
public:
    Discord();
    ~Discord();
    void setStatus(std::string state, std::string details, std::tuple<int, int> partySize);
    void clearStatus();

    void setPlayingStatus(std::shared_ptr<StartInfo> si, const CMap * map, int humanInterfacesCount);
};
