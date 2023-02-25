package eu.vcmi.vcmi.util;

import android.content.Context;
import android.os.Build;

import org.libsdl.app.SDL;

import eu.vcmi.vcmi.NativeMethods;

/**
 * @author F
 */
public final class LibsLoader
{
    private static void loadLib(final String libName, final boolean onlyForOldApis)
    {
        if (!onlyForOldApis || Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP)
        {
            Log.v("Loading native lib: " + libName);
            SDL.loadLibrary(libName);
        }
    }

    private static void loadCommon()
    {
        loadLib("c++_shared", true);
        loadLib("iconv", true);
        loadLib("boost-system", true);
        loadLib("boost-datetime", true);
        loadLib("boost-locale", true);
        loadLib("boost-filesystem", true);
        loadLib("boost-program-options", true);
        loadLib("boost-thread", true);
        loadLib("SDL2", false);
        loadLib("x264", true);
        loadLib("avutil", true);
        loadLib("swscale", true);
        loadLib("swresample", true);
        loadLib("postproc", true);
        loadLib("avcodec", true);
        loadLib("avformat", true);
        loadLib("avfilter", true);
        loadLib("avdevice", true);
        loadLib("minizip", true);
        loadLib("vcmi-fuzzylite", true);
        loadLib("vcmi-lib", true);
        loadLib("SDL2_image", false);
        loadLib("SDL2_mixer", false);
        loadLib("SDL2_ttf", false);
    }

    public static void loadClientLibs(Context ctx)
    {
        loadCommon();
        loadLib("vcmiclient", false);
        SDL.setContext(ctx);
    }

    public static void loadServerLibs()
    {
        loadCommon();
        loadLib("vcmiserver", false);
        NativeMethods.initClassloader();
    }
}
