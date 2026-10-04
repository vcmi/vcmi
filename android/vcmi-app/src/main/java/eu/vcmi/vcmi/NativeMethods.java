package eu.vcmi.vcmi;

import android.content.Context;
import android.os.Build;
import android.os.Messenger;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.util.Base64;

import org.libsdl.app.SDL;
import org.libsdl.app.SDLActivity;

import java.io.File;
import java.lang.ref.WeakReference;
import java.security.KeyStore;
import java.security.cert.X509Certificate;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.Notifications;

/**
 * @author F
 */
public class NativeMethods
{
    private static WeakReference<Messenger> serverMessengerRef;

    // The server runs in a process without an activity, and SDL3 only holds one of those
    private static Context serviceContext;

    public NativeMethods()
    {
    }

    public static void setServiceContext(final Context ctx)
    {
        serviceContext = ctx;
    }

    private static Context context()
    {
        final Context ctx = SDL.getContext();
        return ctx != null ? ctx : serviceContext;
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
        final Context ctx = context();
        String root = Storage.getVcmiDataDir(ctx).getAbsolutePath();

        Log.i("Accessing data root: " + root);
        return root;
    }

    // this path is visible only to this application; we can store base vcmi configs etc. there
    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static String internalDataRoot()
    {
        final Context ctx = context();
        String root = new File(ctx.getFilesDir(), Const.VCMI_DATA_ROOT_FOLDER_NAME).getAbsolutePath();
        Log.i("Accessing internal data root: " + root);
        return root;
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static String applicationId()
    {
        return context().getPackageName();
    }

    /// CA certificates trusted by the system, in PEM format, for TLS connections made by native code
    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static String getSystemCACertificates()
    {
        final StringBuilder result = new StringBuilder();
        try
        {
            final TrustManagerFactory factory = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
            factory.init((KeyStore) null);
            for (final TrustManager manager : factory.getTrustManagers())
            {
                if (!(manager instanceof X509TrustManager))
                {
                    continue;
                }
                for (final X509Certificate certificate : ((X509TrustManager) manager).getAcceptedIssuers())
                {
                    result.append("-----BEGIN CERTIFICATE-----\n");
                    result.append(Base64.encodeToString(certificate.getEncoded(), Base64.DEFAULT));
                    result.append("-----END CERTIFICATE-----\n");
                }
            }
        }
        catch (final Exception e)
        {
            Log.e("Failed to read system CA certificates", e);
        }
        return result.toString();
    }

    /// shown when the game wants the player's attention while it is in the background
    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void showNotification(final String message)
    {
        Notifications.showGameNotification(message);
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
        final Context ctx = context();
        if (Build.VERSION.SDK_INT >= 29) {
            ((Vibrator) ctx.getSystemService(ctx.VIBRATOR_SERVICE)).vibrate(VibrationEffect.createPredefined(VibrationEffect.EFFECT_TICK));
        } else {
            ((Vibrator) ctx.getSystemService(ctx.VIBRATOR_SERVICE)).vibrate(30);
        }
    }

    private static void internalProgressDisplay(final boolean show)
    {
        final Context ctx = context();
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
