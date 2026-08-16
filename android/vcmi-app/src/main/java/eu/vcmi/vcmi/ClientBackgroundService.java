package eu.vcmi.vcmi;

import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Build;
import android.os.IBinder;

import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.Notifications;

/**
 * Keeps the client process alive while the game is not in the foreground.
 * The game itself keeps running without rendering, this service only stops android
 * from killing it and owns the notification that shows the game is still there.
 */
public class ClientBackgroundService extends Service
{
    @Override
    public IBinder onBind(final Intent intent)
    {
        return null;
    }

    @Override
    public int onStartCommand(final Intent intent, final int flags, final int startId)
    {
        Notifications.createChannels(this);
        Notifications.setServiceRunning(true);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE)
        {
            startForeground(Notifications.ID_SERVICE, Notifications.buildServiceNotification(this), ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        }
        else
        {
            startForeground(Notifications.ID_SERVICE, Notifications.buildServiceNotification(this));
        }

        // the game lives in this process - restarting the service without it would be pointless
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy()
    {
        Log.i(this, "background service destroyed");
        Notifications.setServiceRunning(false);
        Notifications.cancelAll(this);
        super.onDestroy();
    }
}
