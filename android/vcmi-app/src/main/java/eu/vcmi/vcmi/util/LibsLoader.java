package eu.vcmi.vcmi.util;

import android.app.Activity;
import android.os.Build;

import org.libsdl.app.SDL;

import eu.vcmi.vcmi.NativeMethods;

/**
 * @author F
 */
public final class LibsLoader
{
    public static final String CLIENT_LIB = "vcmiclient_"
        + (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP ? Build.SUPPORTED_ABIS[0] : Build.CPU_ABI);

    // SDL.loadLibrary() is not public in SDL3 and only wraps System.loadLibrary() unless
    // ReLinker is on the classpath, which it is not here
    public static void loadClientLibs(Activity ctx)
    {
        System.loadLibrary(CLIENT_LIB);
        SDL.setContext(ctx);
    }

    // not used in single-process build
    public static void loadServerLibs()
    {
        System.loadLibrary("vcmiserver");
        NativeMethods.initClassloader();
    }
}
