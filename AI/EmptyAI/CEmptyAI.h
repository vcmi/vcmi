#include "../../AI_Base.h"
#include "../../CCallback.h"
class CEmptyAI : public CGlobalAI
{
	ICallback * cb;
public:
	void init(ICallback * CB);
	void yourTurn();
	void heroKilled(const CGHeroInstance *);
	void heroCreated(const CGHeroInstance *);
	void heroMoved(const HeroMoveDetails &);
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val) {};
	void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID){};
	void tileRevealed(int3 pos){};
	void tileHidden(int3 pos){};
};

#define NAME "EmptyAI 0.1"