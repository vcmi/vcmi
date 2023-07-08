package eu.vcmi.vcmi;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Environment;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
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

    public static native void createServer();

    public static native void notifyServerReady();

    public static native void notifyServerClosed();

    public static native boolean tryToSaveTheGame();

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
    public static void startServer()
    {
        Log.i("Got server create request");
        final Context ctx = SDL.getContext();

        if (!(ctx instanceof VcmiSDLActivity))
        {
            Log.e("Unexpected context... " + ctx);
            return;
        }

        Intent intent = new Intent(ctx, SDLActivity.class);
        intent.setAction(VcmiSDLActivity.NATIVE_ACTION_CREATE_SERVER);
        // I probably do something incorrectly, but sending new intent to the activity "normally" breaks SDL events handling (probably detaches jnienv?)
        // so instead let's call onNewIntent directly, as out context SHOULD be SDLActivity anyway
        ((VcmiSDLActivity) ctx).hackCallNewIntentDirectly(intent);
//        ctx.startActivity(intent);
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void killServer()
    {
        Log.i("Got server close request");

        final Context ctx = SDL.getContext();
        ctx.stopService(new Intent(ctx, ServerService.class));

        Messenger messenger = requireServerMessenger();
        try
        {
            // we need to actually inform client about killing the server, beacuse it needs to unbind service connection before server gets destroyed
            messenger.send(Message.obtain(null, VcmiSDLActivity.SERVER_MESSAGE_SERVER_KILLED));
        }
        catch (RemoteException e)
        {
            Log.w("Connection with client process broken?");
        }
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void onServerReady()
    {
        Log.i("Got server ready msg");
        Messenger messenger = requireServerMessenger();

        try
        {
            messenger.send(Message.obtain(null, VcmiSDLActivity.SERVER_MESSAGE_SERVER_READY));
        }
        catch (RemoteException e)
        {
            Log.w("Connection with client process broken?");
        }
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
        int duration_ms = 30;
        final Context ctx = SDL.getContext();
        if (Build.VERSION.SDK_INT >= 26) {
            ((Vibrator) ctx.getSystemService(ctx.VIBRATOR_SERVICE)).vibrate(VibrationEffect.createOneShot(duration_ms, 5));
        } else {
            ((Vibrator) ctx.getSystemService(ctx.VIBRATOR_SERVICE)).vibrate(duration_ms);
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
