/*
 * mock_scripting_Service.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

namespace scripting
{

class ServiceMock : public Service
{
public:
	MOCK_CONST_METHOD1(performRegistration, void(Services * ));
	MOCK_CONST_METHOD1(run, void(std::shared_ptr<Pool> ));
	MOCK_CONST_METHOD2(run, void(ServerCallback *, std::shared_ptr<Pool> ));
};

}
