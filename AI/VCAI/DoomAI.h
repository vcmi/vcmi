#pragma once

#include "VCAI.h"

class DoomAI :
	public VCAI
{
public:
	DoomAI();
	virtual ~DoomAI();

	virtual void init(std::shared_ptr<CCallback> CB) override;
	virtual void tryRealize(Goals::Build & g) override;
};

