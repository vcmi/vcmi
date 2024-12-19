/*
 * vcmiqt.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include <QtCore/QtGlobal>

#ifdef VCMIQT_STATIC
# define VCMIQT_LINKAGE
#elif defined(VCMIQT_SHARED)
#  define VCMIQT_LINKAGE Q_DECL_EXPORT
#else
#  define VCMIQT_LINKAGE Q_DECL_IMPORT
#endif
