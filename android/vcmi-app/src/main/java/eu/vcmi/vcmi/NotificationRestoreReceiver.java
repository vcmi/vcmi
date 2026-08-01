package eu.vcmi.vcmi;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import eu.vcmi.vcmi.util.Notifications;

/**
 * Since android 14 the notification of a running foreground service can be swiped away.
 * It is the only way to stop the game from outside, so put it back while the service lives on.
 */
public class NotificationRestoreReceiver extends BroadcastReceiver
{
    public static final String ACTION_RESTORE = "eu.vcmi.vcmi.action.RESTORE_NOTIFICATION";

    @Override
    public void onReceive(final Context context, final Intent intent)
    {
        if (ACTION_RESTORE.equals(intent.getAction()))
        {
            Notifications.restoreServiceNotification(context);
        }
    }
}
