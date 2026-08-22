/*
 * ProfilerMacros.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

// Define VCMI_ENABLE_PROFILING to compile in ad-hoc timing instrumentation added
// throughout the codebase, all channeled through the PERF_ONLY(...) / PERF_MEASURE(...)
// macros below. Disabled by default: adds per-call overhead (extra clock reads and
// bookkeeping) not meant for regular/release builds.
#define VCMI_ENABLE_PROFILING

#ifdef VCMI_ENABLE_PROFILING

#include <chrono>

// PERF_ONLY(code) - compiles in profiling-only declarations and statements (stat
// structs, accumulator instances, diagnostic functions, accumulator updates, summary
// logging) that have no production-code content of their own. Expands to nothing when
// profiling is disabled, so none of it exists in a regular build - not even as a stub.
#define PERF_ONLY(...) __VA_ARGS__

// PERF_MEASURE(accumulator, code) - runs `code` and adds its wall-clock duration, in
// microseconds, to `accumulator`. `code` is production code that must always run;
// when profiling is disabled, only the timing wrapper disappears and `code` still
// runs, unwrapped and with no added overhead.
#define PERF_MEASURE(accumulator, code) \
	do \
	{ \
		auto profilingScopeStart = std::chrono::steady_clock::now(); \
		code \
		(accumulator) += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - profilingScopeStart).count(); \
	} while (false)

#else

#define PERF_ONLY(...)
#define PERF_MEASURE(accumulator, code) code

#endif
