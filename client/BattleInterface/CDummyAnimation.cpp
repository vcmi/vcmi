#include "StdInc.h"
#include "CDummyAnimation.h"

#include "CBattleInterface.h"

bool CDummyAnimation::init()
{
	return true;
}

void CDummyAnimation::nextFrame()
{
	counter++;
	if(counter > howMany)
		endAnim();
}

void CDummyAnimation::endAnim()
{
	CBattleAnimation::endAnim();

	delete this;
}

CDummyAnimation::CDummyAnimation(CBattleInterface * _owner, int howManyFrames) : CBattleAnimation(_owner), counter(0), howMany(howManyFrames)
{
}