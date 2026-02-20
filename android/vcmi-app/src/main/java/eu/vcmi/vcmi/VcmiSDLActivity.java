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
    protected String[] getLibraries() {
        // app main library and SDL are loaded when launcher starts, no extra work to do
        return new String[] {
        };
    }

    @Override
    protected String getMainSharedObject() {
        return String.format("%s/lib%s.so", getContext().getApplicationInfo().nativeLibraryDir, LibsLoader.CLIENT_LIB);
    }

    @Override
    protected void onNewIntent(final Intent intent)
    {
        Log.i(this, "Got new intent with action " + intent.getAction());
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        final View outerLayout = getLayoutInflater().inflate(R.layout.activity_game, null, false);
        final ViewGroup layout = (ViewGroup) outerLayout.findViewById(R.id.game_outer_frame);
        mProgressBar = outerLayout.findViewById(R.id.game_progress);

        mLayout.removeView(mSurface);
        layout.addView(mSurface);
        mLayout = layout;

        setContentView(outerLayout);

        VcmiSDLActivity.this.setWindowStyle(true); // set fullscreen
    }

    @Override
    protected void onDestroy()
    {
        unbindServer();

        super.onDestroy();

        finishAffinity();
        System.exit(0);
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
    }
}
