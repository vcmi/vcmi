find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(SDL3_ttf REQUIRED)

vcmi_resolve_sdl3_target(sdl3Image SDL3_image::SDL3_image SDL3_image::SDL3_image-static sdl_image::sdl_image)
vcmi_resolve_sdl3_target(sdl3Mixer SDL3_mixer::SDL3_mixer SDL3_mixer::SDL3_mixer-static sdl_mixer::sdl_mixer)
vcmi_resolve_sdl3_target(sdl3Ttf   SDL3_ttf::SDL3_ttf     SDL3_ttf::SDL3_ttf-static     sdl_ttf::sdl_ttf)

set(vcmisdl3_SRCS
	events/NotificationHandler.cpp
	events/InputHandler.cpp
	events/InputSourceKeyboard.cpp
	events/InputSourceMouse.cpp
	events/InputSourceText.cpp
	events/InputSourceTouch.cpp
	events/InputSourceGameController.cpp
	media/CAudioBase.cpp
	media/CMusicHandler.cpp
	media/CSoundHandler.cpp
	media/CVideoHandler.cpp
	render/CBitmapHandler.cpp
	render/Canvas.cpp
	render/CanvasImage.cpp
	render/GpuResources.cpp
	render/CBitmapFont.cpp
	render/CTrueTypeFont.cpp
	render/CursorHardware.cpp
	render/CursorSoftware.cpp
	render/FontChain.cpp
	render/FrameTimestamps.cpp
	render/GpuProfiler.cpp
	render/ScalableImage.cpp
	render/RenderHandler.cpp
	render/SDLImage.cpp
	render/SDLImageLoader.cpp
	render/SDLImageScaler.cpp
	render/SDLRWwrapper.cpp
	render/ScreenHandler.cpp
	render/SDL_Extensions.cpp
	render/TextTextureCache.cpp
)

set(vcmisdl3_HEADERS
	events/NotificationHandler.h
	events/InputHandler.h
	events/InputSourceKeyboard.h
	events/InputSourceMouse.h
	events/InputSourceText.h
	events/InputSourceTouch.h
	events/InputSourceGameController.h
	media/CAudioBase.h
	media/CEmptyVideoPlayer.h
	media/CMusicHandler.h
	media/CSoundHandler.h
	media/CVideoHandler.h
	media/IMusicPlayer.h
	media/ISoundPlayer.h
	media/IVideoPlayer.h
	render/MemoryBudgetedCache.h
	render/Canvas.h
	render/CanvasImage.h
	render/GpuResources.h
	render/IImage.h
	render/IScreenHandler.h
	render/CBitmapFont.h
	render/CTrueTypeFont.h
	render/CursorHardware.h
	render/CursorSoftware.h
	render/FontChain.h
	render/FrameTimestamps.h
	render/GpuProfiler.h
	render/ScalableImage.h
	render/RenderHandler.h
	render/SDLImage.h
	render/SDLImageLoader.h
	render/SDLImageScaler.h
	render/SDLRWwrapper.h
	render/ScreenHandler.h
	render/SDL_Extensions.h
	render/TextTextureCache.h
	render/SDL_PixelAccess.h
)

assign_source_group(${vcmisdl3_SRCS} ${vcmisdl3_HEADERS})
add_library(vcmisdl3 STATIC ${vcmisdl3_SRCS} ${vcmisdl3_HEADERS})

if(WIN32)
	set_target_properties(vcmisdl3
		PROPERTIES
			OUTPUT_NAME "VCMI_vcmisdl3"
			PROJECT_LABEL "VCMI_vcmisdl3"
	)
	if(NOT ENABLE_DEBUG_CONSOLE AND TARGET SDL3::SDL3main)
		target_link_libraries(vcmisdl3 SDL3::SDL3main)
	endif()
	target_compile_definitions(vcmisdl3 PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)
elseif(IOS)
	target_link_libraries(vcmisdl3 PRIVATE
		iOS_utils

		# FFmpeg
		bz2
		iconv
		z
		"-framework AudioToolbox"
		"-framework AVFoundation"
		"-framework CoreMedia"
		"-framework VideoToolbox"
	)
endif()

# NOTE: the sources here are still the unmodified SDL2 implementation - this is the
# starting point for the SDL3 port, not a working backend yet.
#
# this backend implements interfaces declared by the client and in turn uses client facilities
# such as GameEngine and gui/, so the two static libraries are mutually dependent
target_link_libraries(vcmisdl3 PUBLIC
		vcmiclientcommon vcmi
		${SDL3_CORE_TARGET} ${sdl3Image} ${sdl3Mixer} ${sdl3Ttf}
		libsquish::libsquish
)

if(ENABLE_VIDEO)
	target_compile_definitions(vcmisdl3 PRIVATE ENABLE_VIDEO)
	target_link_libraries(vcmisdl3 PRIVATE
		${ffmpeg_LIBRARIES}
	)
endif()

# this directory is the include root of the project, so its headers are included
# as "render/Canvas.h", "media/ISoundPlayer.h" etc. from anywhere
target_include_directories(vcmisdl3 PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (ffmpeg_INCLUDE_DIRS)
	target_include_directories(vcmisdl3 PRIVATE
		${ffmpeg_INCLUDE_DIRS}
	)
endif()

vcmi_set_output_dir(vcmisdl3 "")
enable_pch(vcmisdl3)
