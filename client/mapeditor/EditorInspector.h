/*
 * EditorInspector.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/constants/EntityIdentifiers.h"

class CArmedInstance;
class CFormView;
class CGArtifact;
class CGCreature;
class CGDwelling;
class CGEvent;
class CGGarrison;
class CGHeroInstance;
class CGHeroPlaceholder;
class CGMine;
class CGObjectInstance;
class CGPandoraBox;
class CGResource;
class CGShipyard;
class CGSignBottle;
class CGTownInstance;
class CLabel;
class CRewardableObject;
class CTextButton;
class EditorMap;
class FlaggableMapObject;
class QuestGuard;
class QuestSource;

/// Panel that shows and edits the properties of a map object
class EditorInspector : public CIntObject
{
	EditorMap & editor;
	CGObjectInstance * object = nullptr;
	ObjectInstanceID objectId;
	int level = 0;

	std::shared_ptr<CLabel> header;
	std::shared_ptr<CFormView> form;

	/// The object, or nullptr if it was removed from the map
	CGObjectInstance * validObject() const;
	void rebuild();
	void changed();

	void addOwner(const std::string & label, PlayerColor & owner, bool editable);
	std::shared_ptr<CTextButton> addTextProperty(const std::string & label, const std::function<std::string()> & getter, const std::function<void(const std::string &)> & setter, bool multiline);
	void addNumberProperty(const std::string & label, int value, int minValue, int maxValue, const std::function<void(int)> & setter);
	void addCheckProperty(const std::string & label, bool value, const std::function<void(bool)> & setter);
	void addChoiceProperty(const std::string & label, const std::vector<std::string> & options, int selected, const std::function<void(int)> & setter);
	void addDialogProperty(const std::string & label, const std::string & buttonText, const std::function<void()> & opener);

	void addProperties(CArmedInstance * o);
	void addProperties(CGResource * o);
	void addProperties(CGArtifact * o);
	void addProperties(CGMine * o);
	void addProperties(CGGarrison * o);
	void addProperties(CGDwelling * o);
	void addProperties(CGTownInstance * o);
	void addProperties(CGCreature * o);
	void addProperties(CGHeroPlaceholder * o);
	void addProperties(CGHeroInstance * o);
	void addProperties(CGSignBottle * o);
	void addProperties(CRewardableObject * o);
	void addProperties(CGPandoraBox * o);
	void addProperties(CGEvent * o);
	void addProperties(QuestSource * o);
	void addProperties(QuestGuard * o);

public:
	/// Called after a property of the object was changed
	std::function<void(CGObjectInstance *)> onObjectChanged;

	EditorInspector(const Rect & area, EditorMap & editor);

	void showObject(CGObjectInstance * newObject);
	void clear();
	/// Reads the properties of the shown object again
	void refresh();
	void setLevel(int newLevel);
};
