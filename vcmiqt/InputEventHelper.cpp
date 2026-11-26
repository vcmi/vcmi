/*
 * InputEventHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "InputEventHelper.h"

#if defined(VCMI_ANDROID) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QApplication>
#include <QMouseEvent>
#include <QTabletEvent>
#include <QWindow>

namespace
{
class InputEventHelper : public QObject
{
public:
	using QObject::QObject;

	bool eventFilter(QObject * obj, QEvent * event) override
	{
		if(auto * window = qobject_cast<QWindow *>(obj))
		{
			// Qt 5 on Android mixes native and logical coordinates for input events
			// delivered to top-level windows. This is normally invisible for the main
			// fullscreen window, but offsets input in popups and dialogs.
			switch(event->type())
			{
				case QEvent::MouseButtonPress:
				case QEvent::MouseButtonRelease:
				case QEvent::MouseMove:
				case QEvent::MouseButtonDblClick:
				{
					auto * mouseEvent = static_cast<QMouseEvent *>(event);
					// screenPos() is the global position in logical coordinates
					QPointF correctedLocal = mouseEvent->screenPos() - QPointF(window->position());
					struct Accessor : QMouseEvent
					{
						void setLocalPosition(const QPointF & position) { l = position; w = position; }
					};
					static_cast<Accessor *>(mouseEvent)->setLocalPosition(correctedLocal);
					return false;
				}
				case QEvent::TabletPress:
				case QEvent::TabletRelease:
				case QEvent::TabletMove:
				{
					auto * tabletEvent = static_cast<QTabletEvent *>(event);
					// only mPos is damaged, globalPosF() went through fromNativePixels
					QPointF correctedLocal = tabletEvent->globalPosF() - QPointF(window->position());
					struct Accessor : QTabletEvent
					{
						void setLocalPosition(const QPointF & position) { mPos = position; }
					};
					static_cast<Accessor *>(tabletEvent)->setLocalPosition(correctedLocal);
					return false;
				}
				default:
					return false;
			}
		}

		return false;
	}
};
}
#endif

void installInputEventHelper(QObject * parent)
{
#if defined(VCMI_ANDROID) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	qApp->installEventFilter(new InputEventHelper(parent));
#else
	Q_UNUSED(parent);
#endif
}
