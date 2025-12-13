package eu.vcmi.vcmi;

import android.content.Context;
import android.os.Build;
import android.os.Messenger;
import android.os.VibrationEffect;
import android.os.Vibrator;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLActivity;

import java.io.File;
import java.lang.ref.WeakReference;

import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public class NativeMethods
{
    private static WeakReference<Messenger> serverMessengerRef;

    public NativeMethods()
    {
    }

    public static native void initClassloader();
    public static native void heroesDataUpdate();

    public static void setupMsg(final Messenger msg)
    {
        serverMessengerRef = new WeakReference<>(msg);
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static String dataRoot()
    {
        final Context ctx = SDL.getContext();
        String root = Storage.getVcmiDataDir(ctx).getAbsolutePath();

        Log.i("Accessing data root: " + root);
        return root;
    }

    // this path is visible only to this application; we can store base vcmi configs etc. there
    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static String internalDataRoot()
    {
        final Context ctx = SDL.getContext();
        String root = new File(ctx.getFilesDir(), Const.VCMI_DATA_ROOT_FOLDER_NAME).getAbsolutePath();
        Log.i("Accessing internal data root: " + root);
        return root;
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static String nativePath()
    {
        final Context ctx = SDL.getContext();
        Log.i("Accessing ndk path: " + ctx.getApplicationInfo().nativeLibraryDir);
        return ctx.getApplicationInfo().nativeLibraryDir;
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void showProgress()
    {
        internalProgressDisplay(true);
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void hideProgress()
    {
        internalProgressDisplay(false);
    }
    
    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void hapticFeedback()
    {
        final Context ctx = SDL.getContext();
        if (Build.VERSION.SDK_INT >= 29) {
            ((Vibrator) ctx.getSystemService(ctx.VIBRATOR_SERVICE)).vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_TICK));
        } else {
            ((Vibrator) ctx.getSystemService(ctx.VIBRATOR_SERVICE)).vibrate(30);
        }
    }

    private static void internalProgressDisplay(final boolean show)
    {
        final Context ctx = SDL.getContext();
        if (!(ctx instanceof VcmiSDLActivity))
        {
            return;
        }
        ((SDLActivity) ctx).runOnUiThread(() -> ((VcmiSDLActivity) ctx).displayProgress(show));
    }

    private static Messenger requireServerMessenger()
    {
        Messenger msg = serverMessengerRef.get();
        if (msg == null)
        {
            throw new RuntimeException("Broken server messenger");
        }
        return msg;
    }
}
