package eu.vcmi.vcmi;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.view.View;
import android.view.ViewGroup;

import org.libsdl.app.SDLActivity;

import eu.vcmi.vcmi.util.LibsLoader;
import eu.vcmi.vcmi.util.Log;

public class VcmiSDLActivity extends SDLActivity
{
    public static final int SERVER_MESSAGE_SERVER_READY = 1000;
    public static final int SERVER_MESSAGE_SERVER_KILLED = 1001;
    public static final String NATIVE_ACTION_CREATE_SERVER = "SDLActivity.Action.CreateServer";
    protected static final int COMMAND_USER = 0x8000;

    final Messenger mClientMessenger = new Messenger(
            new IncomingServerMessageHandler(
                    new OnServerRegisteredCallback()));
    Messenger mServiceMessenger = null;
    boolean mIsServerServiceBound;
    private View mProgressBar;

    private ServiceConnection mServerServiceConnection = new ServiceConnection()
    {
        public void onServiceConnected(ComponentName className,
                                       IBinder service)
        {
            Log.i(this, "Service connection");
            mServiceMessenger = new Messenger(service);
            mIsServerServiceBound = true;

            try
            {
                Message msg = Message.obtain(null, ServerService.CLIENT_MESSAGE_CLIENT_REGISTERED);
                msg.replyTo = mClientMessenger;
                mServiceMessenger.send(msg);
            }
            catch (RemoteException ignored)
            {
            }
        }

        public void onServiceDisconnected(ComponentName className)
        {
            Log.i(this, "Service disconnection");
            mServiceMessenger = null;
        }
    };

    public void hackCallNewIntentDirectly(final Intent intent)
    {
        onNewIntent(intent);
    }

    public void displayProgress(final boolean show)
    {
        if (mProgressBar != null)
        {
            mProgressBar.setVisibility(show ? View.VISIBLE : View.GONE);
        }
    }

    @Override
    public void loadLibraries()
    {
        LibsLoader.loadClientLibs(this);
    }

    @Override
    protected String getMainSharedObject() {
        String library = "libvcmiclient.so";

        return getContext().getApplicationInfo().nativeLibraryDir + "/" + library;
    }

    @Override
    protected void onNewIntent(final Intent intent)
    {
        Log.i(this, "Got new intent with action " + intent.getAction());
        if (NATIVE_ACTION_CREATE_SERVER.equals(intent.getAction()))
        {
            initService();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        if(mBrokenLibraries)
            return;

        final View outerLayout = getLayoutInflater().inflate(R.layout.activity_game, null, false);
        final ViewGroup layout = (ViewGroup) outerLayout.findViewById(R.id.game_outer_frame);
        mProgressBar = outerLayout.findViewById(R.id.game_progress);

        mLayout.removeView(mSurface);
        layout.addView(mSurface);
        mLayout = layout;

        setContentView(outerLayout);
    }

    @Override
    protected void onDestroy()
    {
        try
        {
            // since android can kill the activity unexpectedly (e.g. memory is low or device is inactive for some time), let's try creating
            // an autosave so user might be able to resume the game; this isn't a very good impl (we shouldn't really sleep here and hope that the
            // save is created, but for now it might suffice
            // (better solution: listen for game's confirmation that the save has been created -- this would allow us to inform the users
            // on the next app launch that there is an automatic save that they can use)
            if (NativeMethods.tryToSaveTheGame())
            {
                Thread.sleep(1000L);
            }
        }
        catch (final InterruptedException ignored)
        {
        }

        unbindServer();

        super.onDestroy();
    }

    private void initService()
    {
        unbindServer();
        startService(new Intent(this, ServerService.class));
        bindService(
            new Intent(VcmiSDLActivity.this, ServerService.class),
            mServerServiceConnection,
            Context.BIND_AUTO_CREATE);
    }

    private void unbindServer()
    {
        Log.d(this, "Unbinding server " + mIsServerServiceBound);
        if (mIsServerServiceBound)
        {
            unbindService(mServerServiceConnection);
            mIsServerServiceBound = false;
        }
    }

    private interface IncomingServerMessageHandlerCallback
    {
        void unbindServer();
    }

    private class OnServerRegisteredCallback implements IncomingServerMessageHandlerCallback
    {
        @Override
        public void unbindServer()
        {
            VcmiSDLActivity.this.unbindServer();
        }
    }

    private static class IncomingServerMessageHandler extends Handler
    {
        private VcmiSDLActivity.IncomingServerMessageHandlerCallback mCallback;

        IncomingServerMessageHandler(
                final VcmiSDLActivity.IncomingServerMessageHandlerCallback callback)
        {
            mCallback = callback;
        }

        @Override
        public void handleMessage(Message msg)
        {
            Log.i(this, "Got server msg " + msg);
            switch (msg.what)
            {
                case SERVER_MESSAGE_SERVER_READY:
                    NativeMethods.notifyServerReady();
                    break;
                case SERVER_MESSAGE_SERVER_KILLED:
                    if (mCallback != null)
                    {
                        mCallback.unbindServer();
                    }
                    NativeMethods.notifyServerClosed();
                    break;
                default:
                    super.handleMessage(msg);
            }
        }
    }
}