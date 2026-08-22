/*
 * GpuProfiler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GpuProfiler.h"
#include "GpuResources.h"

#include "CMT.h"

#include <SDL3/SDL_render.h>

#ifdef VCMI_TRACY

#include <SDL3/SDL_video.h>

// Tracy's OpenGL support calls the GL entry points by name and expects the GL types and
// enums to be declared already, but never includes a GL header itself - the desktop and
// OpenGL ES ones disagree about which symbols exist. Spelling out the handful it needs
// keeps this file free of any platform GL header, and the entry points are resolved
// through SDL so that the extension-only ES variants work as well.
namespace vcmigl
{
using GLenum = unsigned int;
using GLint = int;
using GLuint = unsigned int;
using GLsizei = int;
using GLubyte = unsigned char;
using GLint64 = int64_t;
using GLuint64 = uint64_t;

using FnGenQueries = void (*)(GLsizei, GLuint *);
using FnGetQueryiv = void (*)(GLenum, GLenum, GLint *);
using FnGetQueryObjectiv = void (*)(GLuint, GLenum, GLint *);
using FnGetQueryObjectui64v = void (*)(GLuint, GLenum, GLuint64 *);
using FnGetInteger64v = void (*)(GLenum, GLint64 *);
using FnQueryCounter = void (*)(GLuint, GLenum);
using FnGetError = GLenum (*)();
using FnGetString = const GLubyte * (*)(GLenum);
using FnGetStringi = const GLubyte * (*)(GLenum, GLuint);
using FnGetIntegerv = void (*)(GLenum, GLint *);

FnGenQueries genQueries = nullptr;
FnGetQueryiv getQueryiv = nullptr;
FnGetQueryObjectiv getQueryObjectiv = nullptr;
FnGetQueryObjectui64v getQueryObjectui64v = nullptr;
FnGetInteger64v getInteger64v = nullptr;
FnQueryCounter queryCounter = nullptr;
FnGetError getError = nullptr;
FnGetString getString = nullptr;
FnGetStringi getStringi = nullptr;
FnGetIntegerv getIntegerv = nullptr;
}

using vcmigl::GLenum;
using vcmigl::GLint;
using vcmigl::GLint64;
using vcmigl::GLsizei;
using vcmigl::GLubyte;
using vcmigl::GLuint;
using vcmigl::GLuint64;

#define GL_NO_ERROR 0
#define GL_FALSE 0
#define GL_EXTENSIONS 0x1F03
#define GL_TIMESTAMP 0x8E28
#define GL_QUERY_COUNTER_BITS 0x8864
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867

// tells Tracy that glGetStringi may be used to enumerate extensions
#define GL_ES_VERSION_3_0 1

#define glGenQueries ::vcmigl::genQueries
#define glGetQueryiv ::vcmigl::getQueryiv
#define glGetQueryObjectiv ::vcmigl::getQueryObjectiv
#define glGetQueryObjectui64v ::vcmigl::getQueryObjectui64v
#define glGetInteger64v ::vcmigl::getInteger64v
#define glQueryCounter ::vcmigl::queryCounter
#define glGetError ::vcmigl::getError
#define glGetString ::vcmigl::getString
#define glGetStringi ::vcmigl::getStringi
#define glGetIntegerv ::vcmigl::getIntegerv

// same treatment the Tracy sources get in CMake - its headers do not survive -Werror
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <tracy/TracyOpenGL.hpp>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

namespace
{
bool profilerActive = false;
bool profilerInitialized = false;

/// The GL context is current on one thread only, and canvases are occasionally bound from
/// others - where every GPU call, timer queries included, would fail.
std::thread::id renderingThread;

/// Open pass, if any. Held rather than scoped because a pass ends where the next one starts.
std::optional<tracy::GpuCtxScope> currentPass;

/// Zone descriptions handed to Tracy. It keeps the pointers, so these have to stay put -
/// hence a node based container rather than a vector.
std::map<SDL_Texture *, tracy::SourceLocationData> targetNames;

const tracy::SourceLocationData offscreenPass{"GPU: offscreen pass", "GpuProfiler::beginPass", __FILE__, __LINE__, 0};

/// Resolves one entry point, preferring the core name and falling back to the ES extension
template<typename Fn>
bool loadEntryPoint(Fn & function, const char * name, const char * extensionName)
{
	function = reinterpret_cast<Fn>(SDL_GL_GetProcAddress(name));

	if(function == nullptr && extensionName != nullptr)
		function = reinterpret_cast<Fn>(SDL_GL_GetProcAddress(extensionName));

	if(function == nullptr)
		logGlobal->info("GPU profiling unavailable: the driver does not export %s", name);

	return function != nullptr;
}

bool loadEntryPoints()
{
	return loadEntryPoint(vcmigl::genQueries, "glGenQueries", "glGenQueriesEXT")
		&& loadEntryPoint(vcmigl::getQueryiv, "glGetQueryiv", "glGetQueryivEXT")
		&& loadEntryPoint(vcmigl::getQueryObjectiv, "glGetQueryObjectiv", "glGetQueryObjectivEXT")
		&& loadEntryPoint(vcmigl::getQueryObjectui64v, "glGetQueryObjectui64v", "glGetQueryObjectui64vEXT")
		&& loadEntryPoint(vcmigl::getInteger64v, "glGetInteger64v", "glGetInteger64vEXT")
		&& loadEntryPoint(vcmigl::queryCounter, "glQueryCounter", "glQueryCounterEXT")
		&& loadEntryPoint(vcmigl::getError, "glGetError", nullptr)
		&& loadEntryPoint(vcmigl::getString, "glGetString", nullptr)
		&& loadEntryPoint(vcmigl::getStringi, "glGetStringi", nullptr)
		&& loadEntryPoint(vcmigl::getIntegerv, "glGetIntegerv", nullptr);
}

/// Tracy asserts on a driver that reports timer queries but resolves every timestamp to
/// zero, which some tiled GPUs do - so the same question is asked here first.
bool hasUsableTimestamps()
{
	// whatever the renderer left behind would otherwise be read as this query failing
	for(int i = 0; i < 16 && vcmigl::getError() != GL_NO_ERROR; ++i)
		;

	GLint bits = 0;
	vcmigl::getQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &bits);

	// a driver without timer queries reports an error rather than writing anything
	if(vcmigl::getError() != GL_NO_ERROR)
		bits = 0;

	if(bits <= 0)
		logGlobal->info("GPU profiling unavailable: the driver does not implement GL_TIMESTAMP precision");

	return bits > 0;
}

bool isOpenGlRenderer()
{
	const char * name = SDL_GetRendererName(GpuResources::get().renderer());

	if(name == nullptr)
		return false;

	// SDL names its GL backends "opengl", "opengles2" and (historically) "opengles"
	return std::string_view(name).starts_with("opengl");
}
}

void GpuProfiler::initialize()
{
	SDL_Renderer * renderer = GpuResources::get().renderer();

	if(profilerInitialized || renderer == nullptr)
		return;

	profilerInitialized = true;

	if(!isOpenGlRenderer())
	{
		logGlobal->info("GPU profiling unavailable: renderer '%s' is not an OpenGL one", SDL_GetRendererName(renderer));
		return;
	}

	if(!loadEntryPoints() || !hasUsableTimestamps())
		return;

	TracyGpuContext;
	TracyGpuContextName("GPU", 3);

	renderingThread = std::this_thread::get_id();
	profilerActive = true;
	logGlobal->info("GPU profiling enabled");
}

void GpuProfiler::shutdown()
{
	endPass();
	profilerActive = false;
}

void GpuProfiler::nameTarget(SDL_Texture * target, const char * name)
{
	targetNames.insert_or_assign(target, tracy::SourceLocationData{name, "GpuProfiler::beginPass", __FILE__, __LINE__, 0});
}

void GpuProfiler::forgetTarget(SDL_Texture * target)
{
	targetNames.erase(target);
}

void GpuProfiler::beginPass(SDL_Texture * target)
{
	if(!profilerActive || std::this_thread::get_id() != renderingThread)
		return;

	currentPass.reset();

	auto it = targetNames.find(target);
	currentPass.emplace(it != targetNames.end() ? &it->second : &offscreenPass, true);
}

void GpuProfiler::endPass()
{
	if(std::this_thread::get_id() != renderingThread)
		return;

	currentPass.reset();
}

void GpuProfiler::collect()
{
	if(!profilerActive || std::this_thread::get_id() != renderingThread)
		return;

	TracyGpuCollect;
}

bool GpuProfiler::isActive()
{
	return profilerActive;
}

#else

void GpuProfiler::initialize() {}
void GpuProfiler::shutdown() {}
void GpuProfiler::nameTarget(SDL_Texture *, const char *) {}
void GpuProfiler::forgetTarget(SDL_Texture *) {}
void GpuProfiler::beginPass(SDL_Texture *) {}
void GpuProfiler::endPass() {}
void GpuProfiler::collect() {}
bool GpuProfiler::isActive() { return false; }

#endif
