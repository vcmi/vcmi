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
    public static void loadClientLibs(Context ctx)
    {
        SDL.loadLibrary("vcmiclient");
        SDL.setContext(ctx);
    }

    public static void loadServerLibs()
    {
        SDL.loadLibrary("vcmiserver");
        NativeMethods.initClassloader();
    }
}
