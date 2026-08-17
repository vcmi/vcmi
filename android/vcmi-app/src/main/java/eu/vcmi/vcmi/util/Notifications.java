package eu.vcmi.vcmi.util;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

import androidx.annotation.Keep;
import androidx.core.app.NotificationCompat;

import org.libsdl.app.SDL;

import eu.vcmi.vcmi.ActivityQuitConfirm;
import eu.vcmi.vcmi.ClientBackgroundService;
import eu.vcmi.vcmi.Const;
import eu.vcmi.vcmi.NotificationRestoreReceiver;
import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.VcmiSDLActivity;

/**
 * Notifications shown while the game keeps running in the background.
 */
public class Notifications
{
    /** ongoing notification of the service that keeps the game alive */
    public static final String CHANNEL_SERVICE = "vcmi-running";
    /** turn and chat notifications, shown as a popup */
    public static final String CHANNEL_GAME = "vcmi-game-events";

    public static final int ID_SERVICE = 1;
    public static final int ID_GAME = 2;

    private static volatile boolean foreground = false;
    private static volatile boolean serviceRunning = false;

    /**
     * Game events are only worth a notification while the player is looking at something else.
     */
    public static void setForeground(final Context ctx, final boolean value)
    {
        foreground = value;

        if (value)
        {
            manager(ctx).cancel(ID_GAME);
        }
    }

    public static boolean isForeground()
    {
        return foreground;
    }

    public static void createChannels(final Context ctx)
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O)
        {
            return;
        }

        final NotificationChannel service = new NotificationChannel(
            CHANNEL_SERVICE, ctx.getString(R.string.notification_channel_running), NotificationManager.IMPORTANCE_LOW);
        service.setShowBadge(false);

        // IMPORTANCE_HIGH is what makes android show the notification as a popup
        final NotificationChannel game = new NotificationChannel(
            CHANNEL_GAME, ctx.getString(R.string.notification_channel_game), NotificationManager.IMPORTANCE_HIGH);

        manager(ctx).createNotificationChannel(service);
        manager(ctx).createNotificationChannel(game);
    }

    /**
     * The notification the foreground service is tied to - it may not be dismissed by the user,
     * so it carries the only way to stop the game from outside.
     */
    public static Notification buildServiceNotification(final Context ctx)
    {
        final Intent quit = new Intent(ctx, ActivityQuitConfirm.class);
        quit.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);

        return new NotificationCompat.Builder(ctx, CHANNEL_SERVICE)
            .setContentTitle(ctx.getString(R.string.notification_running_title))
            .setContentText(ctx.getString(R.string.notification_running_text))
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentIntent(openGameIntent(ctx))
            .setOngoing(true)
            .setDeleteIntent(pendingBroadcast(ctx, NotificationRestoreReceiver.ACTION_RESTORE, 2))
            .setShowWhen(false)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .addAction(0, ctx.getString(R.string.notification_quit), pendingActivity(ctx, quit, 1))
            .build();
    }

    /**
     * Called by the engine for turn and chat events.
     */
    @Keep
    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    public static void showGameNotification(final String message)
    {
        final Context ctx = SDL.getContext();

        if (ctx == null || foreground || message == null || message.isEmpty())
        {
            return;
        }

        createChannels(ctx);

        final Notification notification = new NotificationCompat.Builder(ctx, CHANNEL_GAME)
            .setContentTitle(ctx.getString(R.string.notification_game_title))
            .setContentText(message)
            .setStyle(new NotificationCompat.BigTextStyle().bigText(message))
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentIntent(openGameIntent(ctx))
            .setAutoCancel(true)
            // channel importance covers android 8 and later, these two do the same below that
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setDefaults(NotificationCompat.DEFAULT_ALL)
            .setCategory(NotificationCompat.CATEGORY_MESSAGE)
            .build();

        try
        {
            manager(ctx).notify(ID_GAME, notification);
        }
        catch (final SecurityException e)
        {
            // notifications were denied by the user - the game itself is not affected
            Log.w("Notifications", "cannot post notification: " + e);
        }
    }

    public static void setServiceRunning(final boolean value)
    {
        serviceRunning = value;
    }

    /**
     * Brings the service notification back after the user swiped it away.
     */
    public static void restoreServiceNotification(final Context ctx)
    {
        if (!serviceRunning)
        {
            return;
        }

        try
        {
            manager(ctx).notify(ID_SERVICE, buildServiceNotification(ctx));
        }
        catch (final SecurityException e)
        {
            Log.w("Notifications", "cannot restore notification: " + e);
        }
    }

    public static void cancelAll(final Context ctx)
    {
        manager(ctx).cancel(ID_GAME);
        manager(ctx).cancel(ID_SERVICE);
    }

    public static void stopService(final Context ctx)
    {
        ctx.stopService(new Intent(ctx, ClientBackgroundService.class));
    }

    private static PendingIntent openGameIntent(final Context ctx)
    {
        final Intent open = new Intent(ctx, VcmiSDLActivity.class);
        open.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);

        return pendingActivity(ctx, open, 0);
    }

    private static PendingIntent pendingActivity(final Context ctx, final Intent intent, final int requestCode)
    {
        int flags = PendingIntent.FLAG_UPDATE_CURRENT;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            flags |= PendingIntent.FLAG_IMMUTABLE;
        }

        return PendingIntent.getActivity(ctx, requestCode, intent, flags);
    }

    private static PendingIntent pendingBroadcast(final Context ctx, final String action, final int requestCode)
    {
        final Intent intent = new Intent(ctx, NotificationRestoreReceiver.class);
        intent.setAction(action);

        int flags = PendingIntent.FLAG_UPDATE_CURRENT;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
        {
            flags |= PendingIntent.FLAG_IMMUTABLE;
        }

        return PendingIntent.getBroadcast(ctx, requestCode, intent, flags);
    }

    private static NotificationManager manager(final Context ctx)
    {
        return (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
    }
}
