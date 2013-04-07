#include "StdInc.h"
#include "ConfigWindows.h"
#include "Editor.h"
#include "../lib/mapping/CMap.h"

#include <QTabWidget>

MapSpecifications::MapSpecifications( QWidget *p /*= 0*/ ) : QDialog(p), editor(dynamic_cast<Editor*>(p))
{
	QTabWidget * tabs = new QTabWidget(this);

	enum {GENERAL, PLAYER_SPEC, TEAMS, RUMORS, TIMED_EVENTS, LOSS_COND, VIC_COND, HEROES, ARTIFACTS, SPELLS, SEC_SKILLS};
	QWidget * tabWidgets[11];
	tabWidgets[GENERAL] = new QWidget(tabs);

	int tabNames[] = {160, 167, 169, 168, 170, 166, 171, 165};

	tabs->addTab(tabWidgets[GENERAL], tr("General"));

}
