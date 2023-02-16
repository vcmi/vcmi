package eu.vcmi.vcmi;

import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.SharedPrefs;

/**
 * @author F
 */
public abstract class ActivityBase extends AppCompatActivity
{
    protected SharedPrefs mPrefs;

    @Override
    protected void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setupExceptionHandler();
        mPrefs = new SharedPrefs(this);
    }

    private void setupExceptionHandler()
    {
        final Thread.UncaughtExceptionHandler prevHandler = Thread.getDefaultUncaughtExceptionHandler();
        if (prevHandler != null && !(prevHandler instanceof VCMIExceptionHandler)) // no need to recreate it if it's already setup
        {
            Thread.setDefaultUncaughtExceptionHandler(new VCMIExceptionHandler(prevHandler));
        }
    }

    private static class VCMIExceptionHandler implements Thread.UncaughtExceptionHandler
    {
        private Thread.UncaughtExceptionHandler mPrevHandler;

        private VCMIExceptionHandler(final Thread.UncaughtExceptionHandler prevHandler)
        {
            mPrevHandler = prevHandler;
        }

        @Override
        public void uncaughtException(final Thread thread, final Throwable throwable)
        {
            Log.e(this, "Unhandled exception", throwable); // to save the exception to file before crashing

            if (mPrevHandler != null && !(mPrevHandler instanceof VCMIExceptionHandler))
            {
                mPrevHandler.uncaughtException(thread, throwable);
            }
            else
            {
                System.exit(1);
            }
        }
    }
}
