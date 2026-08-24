/*
 * ModManagerChecksumTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "StdInc.h"

#include "../../lib/modding/ModManager.h"
#include "../../lib/modding/ModScope.h"

namespace test
{

// Regression test for a shipped bug: ModsState::computeChecksum() hashed files returned by
// an unordered_set without sorting them first, so the checksum for identical content was not
// guaranteed to be reproducible (iteration order depends on hash bucket layout, which is not
// stable run-to-run). This made the persisted "validated checksum" used by
// isModValidationNeeded() unreliable, forcing needless full revalidation of mod content.
// See commit "Fix unstable per-mod checksum for builtin core scope".
//
// This only exercises the in-process determinism guarantee (repeated calls must agree); it does
// not reproduce the specific cross-run self-referential scenario involving modSettings.json,
// which would require swapping out the global "core" resource scope for an isolated fixture.
TEST(ModManagerChecksumTest, CoreChecksumIsStableAcrossCalls)
{
	ModsState modsState;

	uint32_t first = modsState.computeChecksum(ModScope::scopeBuiltin());
	uint32_t second = modsState.computeChecksum(ModScope::scopeBuiltin());

	EXPECT_EQ(first, second);
}

}
