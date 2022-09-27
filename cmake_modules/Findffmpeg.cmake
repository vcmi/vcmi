
#[==[
Provides the following variables:

  * `ffmpeg_INCLUDE_DIRS`: Include directories necessary to use FFMPEG.
  * `ffmpeg_LIBRARIES`: Libraries necessary to use FFMPEG. Note that this only
    includes libraries for the components requested.
  * `ffmpeg_VERSION`: The version of FFMPEG found.

The following components are supported:

  * `avcodec`
  * `avdevice`
  * `avfilter`
  * `avformat`
  * `avresample`
  * `avutil`
  * `swresample`
  * `swscale`

For each component, the following are provided:

  * `ffmpeg_<component>_FOUND`: Libraries for the component.
  * `ffmpeg_<component>_INCLUDE_DIRS`: Include directories for
    the component.
  * `ffmpeg_<component>_LIBRARIES`: Libraries for the component.
  * `ffmpeg::<component>`: A target to use with `target_link_libraries`.

Note that only components requested with `COMPONENTS` or `OPTIONAL_COMPONENTS`
are guaranteed to set these variables or provide targets.
#]==]

function (_ffmpeg_find component headername)
  find_path("ffmpeg_${component}_INCLUDE_DIR"
    NAMES
      "lib${component}/${headername}"
    PATHS
      "${ffmpeg_ROOT}/include"
      ~/Library/Frameworks
      /Library/Frameworks
      /usr/local/include
      /usr/include
      /sw/include # Fink
      /opt/local/include # DarwinPorts
      /opt/csw/include # Blastwave
      /opt/include
      /usr/freeware/include
    PATH_SUFFIXES
      ffmpeg
    DOC "FFMPEG's ${component} include directory")
  mark_as_advanced("ffmpeg_${component}_INCLUDE_DIR")

  # On Windows, static FFMPEG is sometimes built as `lib<name>.a`.
  if (WIN32)
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
    list(APPEND CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
  endif ()

  find_library("ffmpeg_${component}_LIBRARY"
    NAMES
      "${component}"
    PATHS
      "${ffmpeg_ROOT}/lib"
      ~/Library/Frameworks
      /Library/Frameworks
      /usr/local/lib
      /usr/local/lib64
      /usr/lib
      /usr/lib64
      /sw/lib
      /opt/local/lib
      /opt/csw/lib
      /opt/lib
      /usr/freeware/lib64
      "${ffmpeg_ROOT}/bin"
    DOC "FFMPEG's ${component} library")
  mark_as_advanced("ffmpeg_${component}_LIBRARY")

  if (ffmpeg_${component}_LIBRARY AND ffmpeg_${component}_INCLUDE_DIR)
    set(_deps_found TRUE)
    set(_deps_link)
    foreach (_ffmpeg_dep IN LISTS ARGN)
      if (TARGET "ffmpeg::${_ffmpeg_dep}")
        list(APPEND _deps_link "ffmpeg::${_ffmpeg_dep}")
      else ()
        set(_deps_found FALSE)
      endif ()
    endforeach ()
    if (_deps_found)
      if (NOT TARGET "ffmpeg::${component}")
        add_library("ffmpeg::${component}" UNKNOWN IMPORTED)
        set_target_properties("ffmpeg::${component}" PROPERTIES
          IMPORTED_LOCATION "${ffmpeg_${component}_LIBRARY}"
          INTERFACE_INCLUDE_DIRECTORIES "${ffmpeg_${component}_INCLUDE_DIR}"
          IMPORTED_LINK_INTERFACE_LIBRARIES "${_deps_link}")
      endif ()
      set("ffmpeg_${component}_FOUND" 1
        PARENT_SCOPE)

      set(version_header_path "${ffmpeg_${component}_INCLUDE_DIR}/lib${component}/version.h")
      if (EXISTS "${version_header_path}")
        string(TOUPPER "${component}" component_upper)
        file(STRINGS "${version_header_path}" version
          REGEX "#define  *LIB${component_upper}_VERSION_(MAJOR|MINOR|MICRO) ")
        string(REGEX REPLACE ".*_MAJOR *\([0-9]*\).*" "\\1" major "${version}")
        string(REGEX REPLACE ".*_MINOR *\([0-9]*\).*" "\\1" minor "${version}")
        string(REGEX REPLACE ".*_MICRO *\([0-9]*\).*" "\\1" micro "${version}")
        if (NOT major STREQUAL "" AND
            NOT minor STREQUAL "" AND
            NOT micro STREQUAL "")
          set("ffmpeg_${component}_VERSION" "${major}.${minor}.${micro}"
            PARENT_SCOPE)
        endif ()
      endif ()
    else ()
      set("ffmpeg_${component}_FOUND" 0
        PARENT_SCOPE)
      set(what)
      if (NOT ffmpeg_${component}_LIBRARY)
        set(what "library")
      endif ()
      if (NOT ffmpeg_${component}_INCLUDE_DIR)
        if (what)
          string(APPEND what " or headers")
        else ()
          set(what "headers")
        endif ()
      endif ()
      set("ffmpeg_${component}_NOT_FOUND_MESSAGE"
        "Could not find the ${what} for ${component}."
        PARENT_SCOPE)
    endif ()
  endif ()
endfunction ()

_ffmpeg_find(avutil     avutil.h)
_ffmpeg_find(avresample avresample.h
  avutil)
_ffmpeg_find(swresample swresample.h
  avutil)
_ffmpeg_find(swscale    swscale.h
  avutil)
_ffmpeg_find(avcodec    avcodec.h
  avutil)
_ffmpeg_find(avformat   avformat.h
  avcodec avutil)
_ffmpeg_find(avfilter   avfilter.h
  avutil)
_ffmpeg_find(avdevice   avdevice.h
  avformat avutil)

if (TARGET ffmpeg::avutil)
  set(_ffmpeg_version_header_path "${ffmpeg_avutil_INCLUDE_DIR}/libavutil/ffversion.h")
  if (EXISTS "${_ffmpeg_version_header_path}")
    file(STRINGS "${_ffmpeg_version_header_path}" _ffmpeg_version
      REGEX "FFMPEG_VERSION")
    string(REGEX REPLACE ".*\"n?\(.*\)\"" "\\1" ffmpeg_VERSION "${_ffmpeg_version}")
    unset(_ffmpeg_version)
  else ()
    set(ffmpeg_VERSION ffmpeg_VERSION-NOTFOUND)
  endif ()
  unset(_ffmpeg_version_header_path)
endif ()

set(ffmpeg_INCLUDE_DIRS)
set(ffmpeg_LIBRARIES)
set(_ffmpeg_required_vars)
foreach (_ffmpeg_component IN LISTS ffmpeg_FIND_COMPONENTS)
  if (TARGET "ffmpeg::${_ffmpeg_component}")
    set(ffmpeg_${_ffmpeg_component}_INCLUDE_DIRS
      "${ffmpeg_${_ffmpeg_component}_INCLUDE_DIR}")
    set(ffmpeg_${_ffmpeg_component}_LIBRARIES
      "${ffmpeg_${_ffmpeg_component}_LIBRARY}")
    list(APPEND ffmpeg_INCLUDE_DIRS
      "${ffmpeg_${_ffmpeg_component}_INCLUDE_DIRS}")
    list(APPEND ffmpeg_LIBRARIES
      "${ffmpeg_${_ffmpeg_component}_LIBRARIES}")
    if (FFMPEG_FIND_REQUIRED_${_ffmpeg_component})
      list(APPEND _ffmpeg_required_vars
        "ffmpeg_${_ffmpeg_required_vars}_INCLUDE_DIRS"
        "ffmpeg_${_ffmpeg_required_vars}_LIBRARIES")
    endif ()
  endif ()
endforeach ()
unset(_ffmpeg_component)

if (ffmpeg_INCLUDE_DIRS)
  list(REMOVE_DUPLICATES ffmpeg_INCLUDE_DIRS)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ffmpeg
  REQUIRED_VARS ffmpeg_INCLUDE_DIRS ffmpeg_LIBRARIES ${_ffmpeg_required_vars}
  VERSION_VAR ffmpeg_VERSION
  HANDLE_COMPONENTS)
unset(_ffmpeg_required_vars)