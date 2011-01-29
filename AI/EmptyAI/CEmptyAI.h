#include "../../AI_Base.h"
#include "../../CCallback.h"

struct HeroMoveDetails;

class CEmptyAI : public CGlobalAI
{
	ICallback * cb;
public:
	void init(ICallback * CB);
	void yourTurn();
	void heroKilled(const CGHeroInstance *);
	void heroCreated(const CGHeroInstance *);
	void heroMoved(const TryMoveHero&);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val) {};
	void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID){};
	void tileRevealed(int3 pos){};
	void tileHidden(int3 pos){};
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel){};
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd) {};
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
};

#define NAME "EmptyAI 0.1"
