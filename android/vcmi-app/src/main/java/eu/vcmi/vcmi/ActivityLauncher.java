package eu.vcmi.vcmi;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.WindowManager;

import androidx.annotation.Nullable;

import eu.vcmi.vcmi.util.ActivityHelper;
import eu.vcmi.vcmi.util.FileUtil;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import androidx.core.content.FileProvider;

import eu.vcmi.vcmi.VcmiSDLActivity;

import org.libsdl.app.SDL;

/**
 * @author F
 */
public class ActivityLauncher extends org.qtproject.qt5.android.bindings.QtActivity
{
    private static final int PICK_EXTERNAL_VCMI_DATA_TO_COPY = 1;

    public boolean justLaunched = true;

    // set from the Qt thread, so it must not be cached by the UI thread
    private volatile boolean gamepadStartEnabled = false;

    @Override
    public void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        justLaunched = savedInstanceState == null;
        SDL.setContext(this);

        ActivityHelper.applyImmersiveFullscreen(this);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus)
            ActivityHelper.applyImmersiveFullscreen(this);
    }

    public void keepScreenOn(boolean isEnabled)
    {
        if (isEnabled)
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        else
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    /**
     * SDL can't read gamepads from a Qt activity, so the launcher relies on the key events Android delivers here
     */
    @Override
    public boolean dispatchKeyEvent(final KeyEvent event)
    {
        if (event.getAction() == KeyEvent.ACTION_DOWN && isGamepadStartButton(event))
        {
            onLaunchGameBtnPressed();
            return true;
        }

        return super.dispatchKeyEvent(event);
    }

    public void setGamepadStartEnabled(final boolean enabled)
    {
        gamepadStartEnabled = enabled;
    }

    private boolean isGamepadStartButton(final KeyEvent event)
    {
        if (!gamepadStartEnabled)
            return false;

        if (!event.isFromSource(InputDevice.SOURCE_GAMEPAD))
            return false;

        final int keyCode = event.getKeyCode();
        return keyCode == KeyEvent.KEYCODE_BUTTON_A || keyCode == KeyEvent.KEYCODE_BUTTON_START;
    }

    public void onLaunchGameBtnPressed()
    {
        startActivity(new Intent(ActivityLauncher.this, VcmiSDLActivity.class));
    }

    public void openMapEditor()
    {
        startActivity(new Intent(ActivityLauncher.this, ActivityMapEditor.class));
    }

    public void shareFile(String filePath)
    {
        File src = new File(filePath);
        if (!src.exists())
            return;

        // copy to cache so we can share via FileProvider
        File dest = new File(getCacheDir(), src.getName());
        try (InputStream in = new FileInputStream(src); OutputStream out = new FileOutputStream(dest))
        {
            FileUtil.copyStream(in, out);
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return;
        }

        try
        {
            android.net.Uri uri = FileProvider.getUriForFile(this, getPackageName() + ".fileprovider", dest);
            Intent intent = new Intent(Intent.ACTION_SEND);
            intent.setType("application/zip");
            intent.putExtra(Intent.EXTRA_STREAM, uri);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            startActivity(Intent.createChooser(intent, "Share"));
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
