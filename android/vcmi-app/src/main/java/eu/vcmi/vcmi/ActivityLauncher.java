package eu.vcmi.vcmi;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.provider.Settings;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.WindowManager;

import androidx.annotation.Nullable;

import eu.vcmi.vcmi.util.ActivityHelper;
import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.Log;

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
    // qt requests permissions through this activity as well and counts its codes up from 0
    // (nextRequestCode() in qtbase/src/corelib/kernel/qjnihelpers.cpp), so stay out of that range
    private static final int REQUEST_NOTIFICATIONS = 4244;

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

        requestNotificationPermission();
    }

    /**
     * Asked in the launcher so the system dialog can never show up on top of a running game.
     * Denying is not remembered - the next start asks again.
     */
    private void requestNotificationPermission()
    {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU)
        {
            return;
        }

        if (checkSelfPermission(android.Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED)
        {
            return;
        }

        requestPermissions(new String[]{android.Manifest.permission.POST_NOTIFICATIONS}, REQUEST_NOTIFICATIONS);
    }

    @Override
    public void onRequestPermissionsResult(final int requestCode, final String[] permissions, final int[] grantResults)
    {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode != REQUEST_NOTIFICATIONS)
        {
            return;
        }

        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED)
        {
            return;
        }

        // android stops showing its own dialog after the second denial, so offer the settings instead
        new AlertDialog.Builder(this)
            .setTitle(R.string.notification_permission_title)
            .setMessage(R.string.notification_permission_denied)
            .setPositiveButton(R.string.notification_permission_settings, (dialog, which) -> openNotificationSettings())
            .setNegativeButton(android.R.string.ok, null)
            .show();
    }

    private void openNotificationSettings()
    {
        final Intent intent = new Intent(Settings.ACTION_APP_NOTIFICATION_SETTINGS);
        intent.putExtra(Settings.EXTRA_APP_PACKAGE, getPackageName());

        try
        {
            startActivity(intent);
        }
        catch (final ActivityNotFoundException e)
        {
            // the action only exists since android 8
            Log.w(this, "cannot open notification settings: " + e);
        }
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
