/*
 * editorbridge.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "mainwindow.h"

#ifdef ENABLE_SINGLE_APP_BUILD
// Q_INIT_RESOURCE must be called outside any namespace.
// The linker may strip static initializers from OBJECT libraries,
// so we force resource registration here (global namespace).
static void initEditorResources()
{
	Q_INIT_RESOURCE(editor_resources);
	Q_INIT_RESOURCE(editor_translations);
}
#endif

void openMapEditor()
{
#ifdef ENABLE_SINGLE_APP_BUILD
	initEditorResources();
#endif
	auto * editorWindow = new EditorMainWindow();
	editorWindow->setAttribute(Qt::WA_DeleteOnClose);
	editorWindow->show();
}
