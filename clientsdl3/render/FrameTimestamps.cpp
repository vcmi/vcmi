/*
 * FrameTimestamps.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "FrameTimestamps.h"
#include "Profiler.h"

#include <SDL3/SDL_video.h>

#include <deque>

#ifdef VCMI_TRACY

namespace
{
// Spelled out rather than pulled from <EGL/eglext.h>, which is not on the include path of every
// platform this backend builds for. Values are from EGL_ANDROID_get_frame_timestamps.
using EGLint = int32_t;
using EGLBoolean = unsigned int;
using EGLnsecs = int64_t;
using EGLFrameId = uint64_t;

constexpr EGLint EGL_DRAW = 0x3059;
constexpr EGLint EGL_TIMESTAMPS_ANDROID = 0x3430;
constexpr EGLint EGL_COMPOSITE_INTERVAL_ANDROID = 0x3432;
constexpr EGLint EGL_REQUESTED_PRESENT_TIME_ANDROID = 0x3434;
constexpr EGLint EGL_RENDERING_COMPLETE_TIME_ANDROID = 0x3435;
constexpr EGLint EGL_COMPOSITION_LATCH_TIME_ANDROID = 0x3436;
constexpr EGLint EGL_DISPLAY_PRESENT_TIME_ANDROID = 0x343A;
constexpr EGLint EGL_DEQUEUE_READY_TIME_ANDROID = 0x343B;

/// the compositor has not measured this yet - ask again next time
constexpr EGLnsecs EGL_TIMESTAMP_PENDING_ANDROID = -2;
/// the compositor will never report this one
constexpr EGLnsecs EGL_TIMESTAMP_INVALID_ANDROID = -1;

using FnGetCurrentSurface = void * (*)(EGLint);
using FnSurfaceAttrib = EGLBoolean (*)(void *, void *, EGLint, EGLint);
using FnGetNextFrameId = EGLBoolean (*)(void *, void *, EGLFrameId *);
using FnGetFrameTimestamps = EGLBoolean (*)(void *, void *, EGLFrameId, EGLint, const EGLint *, EGLnsecs *);
using FnGetFrameTimestampSupported = EGLBoolean (*)(void *, void *, EGLint);
using FnGetCompositorTiming = EGLBoolean (*)(void *, void *, EGLint, const EGLint *, EGLnsecs *);

FnGetCurrentSurface getCurrentSurface = nullptr;
FnSurfaceAttrib surfaceAttrib = nullptr;
FnGetNextFrameId getNextFrameId = nullptr;
FnGetFrameTimestamps getFrameTimestamps = nullptr;
FnGetFrameTimestampSupported getFrameTimestampSupported = nullptr;
FnGetCompositorTiming getCompositorTiming = nullptr;

void * eglDisplay = nullptr;
void * eglSurface = nullptr;
bool active = false;
bool initialized = false;
bool reportPresentTime = false;
std::thread::id renderingThread;

/// Frames handed to the compositor whose timestamps have not all arrived yet. The compositor is
/// a few frames behind, and it forgets ids that are too old, so this is bounded on both ends.
std::deque<EGLFrameId> pendingFrames;
constexpr size_t maxPendingFrames = 16;

template<typename Fn>
bool loadEntryPoint(Fn & function, const char * name)
{
	function = reinterpret_cast<Fn>(SDL_EGL_GetProcAddress(name));

	if(function == nullptr)
		logGlobal->info("Frame timestamps unavailable: the driver does not export %s", name);

	return function != nullptr;
}

/// Reads one finished frame, or reports that it is not finished yet
enum class ReadResult
{
	DONE,
	PENDING,
	FAILED
};

ReadResult reportFrame(EGLFrameId frameId)
{
	const std::array<EGLint, 5> names = {
		EGL_REQUESTED_PRESENT_TIME_ANDROID,
		EGL_RENDERING_COMPLETE_TIME_ANDROID,
		EGL_COMPOSITION_LATCH_TIME_ANDROID,
		EGL_DEQUEUE_READY_TIME_ANDROID,
		EGL_DISPLAY_PRESENT_TIME_ANDROID,
	};

	std::array<EGLnsecs, 5> values = {};
	const EGLint wanted = reportPresentTime ? 5 : 4;

	if(!getFrameTimestamps(eglDisplay, eglSurface, frameId, wanted, names.data(), values.data()))
		return ReadResult::FAILED;

	for(EGLint i = 0; i < wanted; ++i)
	{
		if(values[i] == EGL_TIMESTAMP_PENDING_ANDROID)
			return ReadResult::PENDING;

		if(values[i] == EGL_TIMESTAMP_INVALID_ANDROID)
			return ReadResult::FAILED;
	}

	const auto ms = [](EGLnsecs from, EGLnsecs to) { return static_cast<double>(to - from) / 1e6; };

	// our own drawing: from asking for the frame to the GPU being done with it
	VCMI_PROFILE_PLOT_FLOAT("Frame: draw to render done (ms)", ms(values[0], values[1]));
	// the compositor holding a buffer that was already finished - the number that tells a
	// withheld frame apart from a late one
	VCMI_PROFILE_PLOT_FLOAT("Frame: render done to latch (ms)", ms(values[1], values[2]));
	// how long until the buffer came back to us, which is what a blocking swap waits for
	VCMI_PROFILE_PLOT_FLOAT("Frame: draw to buffer released (ms)", ms(values[0], values[3]));

	if(reportPresentTime)
	{
		VCMI_PROFILE_PLOT_FLOAT("Frame: latch to display (ms)", ms(values[2], values[4]));
		VCMI_PROFILE_PLOT_FLOAT("Frame: total latency (ms)", ms(values[0], values[4]));
	}

	return ReadResult::DONE;
}

void reportCompositorInterval()
{
	if(getCompositorTiming == nullptr)
		return;

	const std::array<EGLint, 1> names = {EGL_COMPOSITE_INTERVAL_ANDROID};
	std::array<EGLnsecs, 1> values = {};

	if(getCompositorTiming(eglDisplay, eglSurface, 1, names.data(), values.data()) && values[0] > 0)
		VCMI_PROFILE_PLOT_FLOAT("Frame: compositor interval (ms)", static_cast<double>(values[0]) / 1e6);
}
}

void FrameTimestamps::initialize(SDL_Window * window)
{
	if(initialized || window == nullptr)
		return;

	initialized = true;

	eglDisplay = SDL_EGL_GetCurrentDisplay();

	// SDL only hands out the window surface on the desktop backends - its Android driver has no
	// GL_GetEGLSurface at all - so the surface is taken from the context that is current here
	if(loadEntryPoint(getCurrentSurface, "eglGetCurrentSurface"))
		eglSurface = getCurrentSurface(EGL_DRAW);

	if(eglSurface == nullptr)
		eglSurface = SDL_EGL_GetWindowSurface(window);

	if(eglDisplay == nullptr || eglSurface == nullptr)
	{
		logGlobal->info("Frame timestamps unavailable: the window is not EGL backed");
		return;
	}

	const bool loaded = loadEntryPoint(surfaceAttrib, "eglSurfaceAttrib")
		&& loadEntryPoint(getNextFrameId, "eglGetNextFrameIdANDROID")
		&& loadEntryPoint(getFrameTimestamps, "eglGetFrameTimestampsANDROID")
		&& loadEntryPoint(getFrameTimestampSupported, "eglGetFrameTimestampSupportedANDROID");

	if(!loaded)
		return;

	// optional, only used to record the refresh interval the compositor actually runs at
	loadEntryPoint(getCompositorTiming, "eglGetCompositorTimingANDROID");

	if(!surfaceAttrib(eglDisplay, eglSurface, EGL_TIMESTAMPS_ANDROID, 1))
	{
		logGlobal->info("Frame timestamps unavailable: the surface refused to collect them");
		return;
	}

	// scanout time needs support from the display controller, which not every device has -
	// without it the other three still answer the question
	reportPresentTime = getFrameTimestampSupported(eglDisplay, eglSurface, EGL_DISPLAY_PRESENT_TIME_ANDROID) != 0;

	renderingThread = std::this_thread::get_id();
	active = true;
	logGlobal->info("Frame timestamps enabled%s", reportPresentTime ? "" : " (without display present time)");

	reportCompositorInterval();
}

void FrameTimestamps::shutdown()
{
	pendingFrames.clear();
	active = false;
}

void FrameTimestamps::beginFrame()
{
	if(!active || std::this_thread::get_id() != renderingThread)
		return;

	EGLFrameId frameId = 0;
	if(!getNextFrameId(eglDisplay, eglSurface, &frameId))
		return;

	// the compositor drops ids that fall too far behind, so an unanswered one is given up on
	if(pendingFrames.size() >= maxPendingFrames)
		pendingFrames.pop_front();

	pendingFrames.push_back(frameId);
}

void FrameTimestamps::collect()
{
	if(!active || std::this_thread::get_id() != renderingThread)
		return;

	// oldest first - once one is still pending, every later frame is too
	while(!pendingFrames.empty())
	{
		ReadResult result = reportFrame(pendingFrames.front());

		if(result == ReadResult::PENDING)
			break;

		pendingFrames.pop_front();
	}
}

bool FrameTimestamps::isActive()
{
	return active;
}

#else

void FrameTimestamps::initialize(SDL_Window *) {}
void FrameTimestamps::shutdown() {}
void FrameTimestamps::beginFrame() {}
void FrameTimestamps::collect() {}
bool FrameTimestamps::isActive() { return false; }

#endif
