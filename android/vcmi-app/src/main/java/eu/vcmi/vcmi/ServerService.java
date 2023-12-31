package eu.vcmi.vcmi;

import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;

import org.libsdl.app.SDL;

import java.lang.ref.WeakReference;

import eu.vcmi.vcmi.util.LibsLoader;
import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public class ServerService extends Service
{
    public static final int CLIENT_MESSAGE_CLIENT_REGISTERED = 1;
    public static final String INTENT_ACTION_KILL_SERVER = "ServerService.Action.Kill";
    final Messenger mMessenger = new Messenger(new IncomingClientMessageHandler(new OnClientRegisteredCallback()));
    private Messenger mClient;

    @Override
    public IBinder onBind(Intent intent)
    {
        return mMessenger.getBinder();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        SDL.setContext(ServerService.this);
        LibsLoader.loadServerLibs();
        if (INTENT_ACTION_KILL_SERVER.equals(intent.getAction()))
        {
            stopSelf();
        }
        return START_NOT_STICKY;
    }

    @Override
    public void onDestroy()
    {
        super.onDestroy();
        Log.i(this, "destroyed");
        // we need to kill the process to ensure all server data is cleaned up; this isn't a good solution (as we mess with system's
        // memory management stuff), but clearing all native data manually would be a pain and we can't force close the server "gracefully", because
        // even after onDestroy call, the system can postpone actually finishing the process -- this would break CVCMIServer initialization
        System.exit(0);
    }

    private interface IncomingClientMessageHandlerCallback
    {
        void onClientRegistered(Messenger client);
    }

    private static class ServerStartThread extends Thread
    {
        @Override
        public void run()
        {
            NativeMethods.createServer();
        }
    }

    private static class IncomingClientMessageHandler extends Handler
    {
        private WeakReference<IncomingClientMessageHandlerCallback> mCallbackRef;

        IncomingClientMessageHandler(final IncomingClientMessageHandlerCallback callback)
        {
            mCallbackRef = new WeakReference<>(callback);
        }

        @Override
        public void handleMessage(Message msg)
        {
            switch (msg.what)
            {
                case CLIENT_MESSAGE_CLIENT_REGISTERED:
                    final IncomingClientMessageHandlerCallback callback = mCallbackRef.get();
                    if (callback != null)
                    {
                        callback.onClientRegistered(msg.replyTo);
                    }
                    NativeMethods.setupMsg(msg.replyTo);
                    new ServerStartThread().start();
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }

    private class OnClientRegisteredCallback implements IncomingClientMessageHandlerCallback
    {
        @Override
        public void onClientRegistered(final Messenger client)
        {
            mClient = client;
        }
    }
}
