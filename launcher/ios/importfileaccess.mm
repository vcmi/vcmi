/*
 * importfileaccess.mm, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../helper.h"

#import <Foundation/Foundation.h>

namespace
{
/// Keeps the URL alive under ARC until the last import owner releases its access.
class ImportFileAccess final
{
	NSURL * __strong url;
	const bool scoped;

public:
	explicit ImportFileAccess(NSURL * source)
		: url(source)
		, scoped([source startAccessingSecurityScopedResource])
	{
	}

	ImportFileAccess(const ImportFileAccess &) = delete;
	ImportFileAccess & operator=(const ImportFileAccess &) = delete;

	~ImportFileAccess()
	{
		if(scoped)
			[url stopAccessingSecurityScopedResource];
	}
};
}

std::shared_ptr<void> Helper::acquireImportAccess(const QString & path)
{
	return std::make_shared<ImportFileAccess>([NSURL fileURLWithPath:path.toNSString()]);
}
