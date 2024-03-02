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
    public static final String CLIENT_LIB = "vcmiclient_"
        + (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP ? Build.SUPPORTED_ABIS[0] : Build.CPU_ABI);

    public static void loadClientLibs(Context ctx)
    {
        SDL.loadLibrary(CLIENT_LIB);
        SDL.setContext(ctx);
    }

    // not used in single-process build
    public static void loadServerLibs()
    {
        SDL.loadLibrary("vcmiserver");
        NativeMethods.initClassloader();
    }
}
