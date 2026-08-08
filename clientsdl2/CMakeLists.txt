find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
if(TARGET SDL2_image::SDL2_image)
	add_library(SDL2::Image ALIAS SDL2_image::SDL2_image)
endif()
if(TARGET SDL2_image::SDL2_image-static)
	add_library(SDL2::Image ALIAS SDL2_image::SDL2_image-static)
endif()
find_package(SDL2_mixer REQUIRED)
if(TARGET SDL2_mixer::SDL2_mixer)
	add_library(SDL2::Mixer ALIAS SDL2_mixer::SDL2_mixer)
endif()
if(TARGET SDL2_mixer::SDL2_mixer-static)
	add_library(SDL2::Mixer ALIAS SDL2_mixer::SDL2_mixer-static)
endif()
find_package(SDL2_ttf REQUIRED)
if(TARGET SDL2_ttf::SDL2_ttf)
	add_library(SDL2::TTF ALIAS SDL2_ttf::SDL2_ttf)
endif()
if(TARGET SDL2_ttf::SDL2_ttf-static)
	add_library(SDL2::TTF ALIAS SDL2_ttf::SDL2_ttf-static)
endif()

set(vcmisdl2_SRCS
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
	render/CBitmapFont.cpp
	render/CTrueTypeFont.cpp
	render/CursorHardware.cpp
	render/CursorSoftware.cpp
	render/FontChain.cpp
	render/ScalableImage.cpp
	render/RenderHandler.cpp
	render/SDLImage.cpp
	render/SDLImageLoader.cpp
	render/SDLImageScaler.cpp
	render/SDLRWwrapper.cpp
	render/ScreenHandler.cpp
	render/SDL_Extensions.cpp
)

set(vcmisdl2_HEADERS
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
	render/Canvas.h
	render/CanvasImage.h
	render/IImage.h
	render/IScreenHandler.h
	render/CBitmapFont.h
	render/CTrueTypeFont.h
	render/CursorHardware.h
	render/CursorSoftware.h
	render/FontChain.h
	render/ScalableImage.h
	render/RenderHandler.h
	render/SDLImage.h
	render/SDLImageLoader.h
	render/SDLImageScaler.h
	render/SDLRWwrapper.h
	render/ScreenHandler.h
	render/SDL_Extensions.h
	render/SDL_PixelAccess.h
)

assign_source_group(${vcmisdl2_SRCS} ${vcmisdl2_HEADERS})
add_library(vcmisdl2 STATIC ${vcmisdl2_SRCS} ${vcmisdl2_HEADERS})

if(WIN32)
	set_target_properties(vcmisdl2
		PROPERTIES
			OUTPUT_NAME "VCMI_vcmisdl2"
			PROJECT_LABEL "VCMI_vcmisdl2"
	)
	if(NOT ENABLE_DEBUG_CONSOLE)
		target_link_libraries(vcmisdl2 SDL2::SDL2main)
	endif()
	target_compile_definitions(vcmisdl2 PRIVATE WINDOWS_IGNORE_PACKING_MISMATCH)
elseif(IOS)
	target_link_libraries(vcmisdl2 PRIVATE
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

# this backend implements interfaces declared by the client and in turn uses client facilities
# such as GameEngine and gui/, so the two static libraries are mutually dependent
target_link_libraries(vcmisdl2 PUBLIC
		vcmiclientcommon vcmi SDL2::SDL2 SDL2::Image SDL2::Mixer SDL2::TTF libsquish::libsquish
)

if(ENABLE_VIDEO)
	target_compile_definitions(vcmisdl2 PRIVATE ENABLE_VIDEO)
	target_link_libraries(vcmisdl2 PRIVATE
		${ffmpeg_LIBRARIES}
	)
endif()

# this directory is the include root of the project, so its headers are included
# as "render/Canvas.h", "media/ISoundPlayer.h" etc. from anywhere
target_include_directories(vcmisdl2 PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

if (ffmpeg_INCLUDE_DIRS)
	target_include_directories(vcmisdl2 PRIVATE
		${ffmpeg_INCLUDE_DIRS}
	)
endif()

vcmi_set_output_dir(vcmisdl2 "")
enable_pch(vcmisdl2)
