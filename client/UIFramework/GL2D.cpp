#include "StdInc.h"
#include <SDL.h>
#include <SDL_video.h>
#include "SDL_syswm.h"
#include <SDL_opengl.h>
#include "GL2D.h"


namespace GL2D
{

// Variables initialized by initVideo, updated by setScreenRes:
static SDL_SysWMinfo wmInfo;
ui32 screenWidth = 0, screenHeight = 0; // Screen/Window size

// OpenGL 1.3 functions pointers
PFNGLACTIVETEXTUREPROC	glActiveTexture;
// OpenGL 2.0 functions pointers
PFNGLCREATEPROGRAMPROC	glCreateProgram;
PFNGLCREATESHADERPROC	glCreateShader;
PFNGLDELETESHADERPROC	glDeleteShader;
PFNGLSHADERSOURCEPROC	glShaderSource;
PFNGLCOMPILESHADERPROC	glCompileShader;
PFNGLGETSHADERIVPROC	glGetShaderiv;
PFNGLATTACHSHADERPROC	glAttachShader;
PFNGLLINKPROGRAMPROC	glLinkProgram;
PFNGLGETPROGRAMIVPROC	glGetProgramiv;
PFNGLUSEPROGRAMPROC 	glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1IPROC		glUniform1i;
PFNGLUNIFORM2IPROC		glUniform2i;

PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;


// Shaders' sources
static const char frag_palette_bitmap[] = (
"uniform usampler2DRect bitmap;"
"uniform sampler1D palette;"
"uniform ivec2 coordOffs;"
"layout (origin_upper_left) in vec4 gl_FragCoord;"

"void main(){"
	"gl_FragColor = texelFetch(palette, int(texelFetch(bitmap, ivec2(gl_FragCoord.xy) - coordOffs).r), 0);"
"}"
);


// Programs' handlers
static GLuint currentProgram = 0;
static GLuint colorizeProgram = 0;
static GLuint paletteBitmapProgram = 0;

// Uniforms handlers
static GLint coord_uniform = -1;


// Print out the information log for a shader object 
void printInfoLog(PFNGLGETPROGRAMINFOLOGPROC logFunc, GLuint object, GLint status)
{
	if (logFunc != nullptr)
	{
		GLsizei infoLogLength = 0;
		GLchar infoLog[1024];
		logFunc(object, sizeof(infoLog)-1, &infoLogLength, infoLog);

		if (infoLogLength > 0) (status ? tlog0 : tlog1) << infoLog;
	}
}


GLuint makeShaderProgram(const char * frg_src)
{
	// Creating a fragment shader object
	GLuint shader_object = glCreateShader(GL_FRAGMENT_SHADER);
	if (shader_object == 0) return 0;

	glShaderSource(shader_object, 1, &frg_src, nullptr); // assigning the shader source
	glCompileShader(shader_object); // Compiling the shader

	GLint ret;
	glGetShaderiv(shader_object, GL_COMPILE_STATUS, &ret);
	printInfoLog(glGetShaderInfoLog, shader_object, ret);

	if (ret == GL_TRUE)
	{
		GLuint program_object  = glCreateProgram(); // Creating a program object
		glAttachShader(program_object, shader_object); // Attaching the shader into program

		// Link the shaders into a complete GLSL program.
		glLinkProgram(program_object);

		glGetProgramiv(shader_object, GL_LINK_STATUS, &ret);
		printInfoLog(glGetProgramInfoLog, program_object, ret);

		if (ret == GL_TRUE) return program_object;
	}

	glDeleteShader(shader_object);
	return 0;
}


void initVideo(ui32 w, ui32 h, bool fullscreen)
{
	SDL_VERSION(&wmInfo.version);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if (!setScreenRes(w, h, fullscreen))
	{
		throw std::runtime_error("Video mode setting failed\n");
	}
	glClearColor(0, 0, 0, 0);	// Black Background
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	// Dynamic-linked OpenGL 1.3 and 2.0 functions - required to 2D rendeing
	if (	(glActiveTexture	= (PFNGLACTIVETEXTUREPROC)	SDL_GL_GetProcAddress("glActiveTexture")) == nullptr
		||	(glCreateProgram	= (PFNGLCREATEPROGRAMPROC)	SDL_GL_GetProcAddress("glCreateProgram")) == nullptr
		||	(glCreateShader 	= (PFNGLCREATESHADERPROC)	SDL_GL_GetProcAddress("glCreateShader")) == nullptr
		||	(glDeleteShader		= (PFNGLDELETESHADERPROC)	SDL_GL_GetProcAddress("glDeleteShader")) == nullptr
		||	(glShaderSource 	= (PFNGLSHADERSOURCEPROC)	SDL_GL_GetProcAddress("glShaderSource")) == nullptr
		||	(glCompileShader	= (PFNGLCOMPILESHADERPROC)	SDL_GL_GetProcAddress("glCompileShader")) == nullptr
		||	(glGetShaderiv		= (PFNGLGETSHADERIVPROC)	SDL_GL_GetProcAddress("glGetShaderiv")) == nullptr
		||	(glAttachShader 	= (PFNGLATTACHSHADERPROC)	SDL_GL_GetProcAddress("glAttachShader")) == nullptr
		||	(glLinkProgram		= (PFNGLLINKPROGRAMPROC)	SDL_GL_GetProcAddress("glLinkProgram")) == nullptr
		||	(glGetProgramiv 	= (PFNGLGETPROGRAMIVPROC)	SDL_GL_GetProcAddress("glGetProgramiv")) == nullptr
		||	(glUseProgram		= (PFNGLUSEPROGRAMPROC) 	SDL_GL_GetProcAddress("glUseProgram")) == nullptr
		||	(glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) SDL_GL_GetProcAddress("glGetUniformLocation")) == nullptr
		||	(glUniform1i		= (PFNGLUNIFORM1IPROC)		SDL_GL_GetProcAddress("glUniform1i")) == nullptr
		||	(glUniform2i		= (PFNGLUNIFORM2IPROC)		SDL_GL_GetProcAddress("glUniform2i")) == nullptr
		)
	{
		tlog1 << "Error: OpenGL2 Extenstions are not available\n";
		tlog1 << "SDL says: " << SDL_GetError() << std::endl;
		throw std::runtime_error("OpenGL2 Extenstions are not available\n");
	}
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) SDL_GL_GetProcAddress("glGetProgramInfoLog"); // not required
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) SDL_GL_GetProcAddress("glGetShaderInfoLog"); // not required

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_DITHER);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_RECTANGLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	paletteBitmapProgram = makeShaderProgram(frag_palette_bitmap);
	if (paletteBitmapProgram == 0)
	{
		throw std::runtime_error("OpenGL shader for palleted bitmaps is not compiled\n");
	}

	GLint bitmap_uniform = glGetUniformLocation(paletteBitmapProgram, "bitmap");
	GLint palette_uniform = glGetUniformLocation(paletteBitmapProgram, "palette");
	coord_uniform = glGetUniformLocation(paletteBitmapProgram, "coordOffs");

	glUseProgram(currentProgram = paletteBitmapProgram);
	glUniform1i(bitmap_uniform, 0);
	glUniform1i(palette_uniform, 1);

#ifdef _WIN32
	wglMakeCurrent(NULL, NULL);
#endif
}


void attachToCurrentThread()
{
#ifdef _WIN32
	HDC hdc = GetDC(wmInfo.window);
	wglMakeCurrent(hdc, wmInfo.hglrc);
#endif
}


bool setScreenRes(ui32 w, ui32 h, bool fullscreen)
{
	// Try to use the best screen depth for the display
	int suggestedBpp = SDL_VideoModeOK(w, h, 32, SDL_OPENGL | SDL_ANYFORMAT | (fullscreen?SDL_FULLSCREEN:0));
	if(suggestedBpp == 0)
	{
		tlog1 << "Error: SDL says that " << w << "x" << h << " resolution is not available!\n";
		return false;
	}
	
	if(suggestedBpp != 32)
	{
		tlog2 << "Note: SDL suggests to use " << suggestedBpp << " bpp instead of 32 bpp\n";
	}

	if(SDL_SetVideoMode(w, h, suggestedBpp, SDL_OPENGL | SDL_ANYFORMAT | (fullscreen?SDL_FULLSCREEN:0)) == NULL)
	{
		tlog1 << "Error: Video mode setting failed (" << w << "x" << h << "x" << suggestedBpp << "bpp)\n";
		return false;
	}

	screenWidth = w; screenHeight = h;

	int getwm = SDL_GetWMInfo(&wmInfo);
	if(getwm != 1)
	{
		tlog2 << "Something went wrong, getwm=" << getwm << std::endl;
		tlog2 << "SDL says: " << SDL_GetError() << std::endl;
		return false;
	}

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);     // Select The Projection Matrix
	glLoadIdentity();                // Reset The Projection Matrix
	glOrtho(0, w, h, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);      // Select The Modelview Matrix
	glLoadIdentity();                // Reset The Modelview Matrix
	glTranslatef(0.375, 0.375, 0);   // Displacement trick for exact pixelization
	
	return true;
}


void assignTexture(ui32 slot, ui32 type, ui32 handle)
{
	glActiveTexture(slot);
	glBindTexture(type, handle);
}


void useNoShader()
{
	if (currentProgram != 0) {
		glUseProgram(currentProgram = 0);
	}
}


void useColorizeShader(const float cm[4][4])
{
	if (currentProgram != colorizeProgram) {
		glUseProgram(currentProgram = colorizeProgram);
	}
}


void usePaletteBitmapShader(si32 x, si32 y)
{
	if (currentProgram != paletteBitmapProgram) {
		glUseProgram(currentProgram = paletteBitmapProgram);
	}
	glUniform2i(coord_uniform, x, y);
}


void usePaletteBitmapShader(si32 x, si32 y, const float cm[4][4])
{
	if (currentProgram != paletteBitmapProgram) {
		glUseProgram(currentProgram = paletteBitmapProgram);
	}
	glUniform2i(coord_uniform, x, y);
}

}
