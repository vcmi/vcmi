package eu.vcmi.vcmi;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.util.DisplayMetrics;
import androidx.annotation.Nullable;

import eu.vcmi.vcmi.util.ActivityHelper;
import org.libsdl.app.SDL;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.Locale;
import java.util.concurrent.CountDownLatch;

import eu.vcmi.vcmi.util.FileUtil;

public class ActivityMapEditor extends org.qtproject.qt5.android.bindings.QtActivity
{
	// Prevents prepareAndroid() from re-copying APK assets (launcher already did it)
	public boolean justLaunched = false;

	private static final int MIN_UI_HEIGHT_PX = 600;
	private static final int REQUEST_OPEN_FILE = 200;
	private static final int REQUEST_SAVE_FILE = 201;

	private CountDownLatch pickerLatch;
	private String pickerResultUri;

	@Override
	public void onCreate(@Nullable final Bundle savedInstanceState)
	{
		// All env vars must be set before super.onCreate() which starts
		// the QtThread that calls native main()
		try
		{
			Os.setenv("VCMI_LAUNCH_MAP_EDITOR", "1", true);

			// Qt logical height = physical_px / (density * QT_SCALE_FACTOR)
			// We need logical height >= MIN_UI_HEIGHT_PX, so:
			// QT_SCALE_FACTOR <= physical_px / (density * MIN_UI_HEIGHT_PX)
			DisplayMetrics dm = getResources().getDisplayMetrics();
			int screenHeightPx = Math.min(dm.widthPixels, dm.heightPixels);
			double scale = Math.min(1.0, (double) screenHeightPx / (dm.density * MIN_UI_HEIGHT_PX));
			Os.setenv("QT_SCALE_FACTOR", String.format(Locale.US, "%.4f", scale), true);
		}
		catch (ErrnoException e)
		{
			throw new RuntimeException("Failed to set environment", e);
		}
		// NativeMethods.dataRoot() etc. use SDL.getContext() to resolve file paths
		SDL.setContext(this);
		super.onCreate(savedInstanceState);

		ActivityHelper.applyImmersiveFullscreen(this);
	}

	@Override
	public void onWindowFocusChanged(boolean hasFocus)
	{
		super.onWindowFocusChanged(hasFocus);
		if (hasFocus)
			ActivityHelper.applyImmersiveFullscreen(this);
	}

	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		finishAffinity();
		System.exit(0);
	}

	/**
	 * Called from C++ (Qt thread) via JNI.
	 * Opens the Android native file picker for reading.
	 * Blocks the calling (Qt) thread until the user picks a file or cancels.
	 * The picked file is copied to a temp location and the local path is returned.
	 *
	 * @param mimeType MIME type filter, e.g. "*​/*"
	 * @return absolute path to the temp copy, or empty string on cancel/error
	 */
	public String pickFileForOpen(String mimeType)
	{
		pickerLatch = new CountDownLatch(1);
		pickerResultUri = null;

		runOnUiThread(() -> {
			Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
			intent.addCategory(Intent.CATEGORY_OPENABLE);
			intent.setType(mimeType);
			startActivityForResult(intent, REQUEST_OPEN_FILE);
		});

		try { pickerLatch.await(); } catch (InterruptedException ignored) {}

		if (pickerResultUri == null || pickerResultUri.isEmpty())
			return "";

		// Copy the content:// URI to a temp file so Qt can work with a normal path
		try
		{
			Uri uri = Uri.parse(pickerResultUri);
			String displayName = FileUtil.getFilenameFromUri(pickerResultUri, this);
			if (displayName.isEmpty())
				displayName = "picked_file";
			File tempFile = new File(getCacheDir(), displayName);
			try (InputStream in = getContentResolver().openInputStream(uri);
				 OutputStream out = new FileOutputStream(tempFile))
			{
				FileUtil.copyStream(in, out);
			}
			return tempFile.getAbsolutePath();
		}
		catch (IOException e)
		{
			e.printStackTrace();
			return "";
		}
	}

	/**
	 * Called from C++ (Qt thread) via JNI.
	 * Opens the Android native file picker for saving (ACTION_CREATE_DOCUMENT).
	 * Blocks the calling (Qt) thread until the user picks a location or cancels.
	 *
	 * @param mimeType      MIME type for the new file
	 * @param suggestedName default file name shown to the user
	 * @return content:// URI string, or empty string on cancel
	 */
	public String pickFileForSave(String mimeType, String suggestedName)
	{
		pickerLatch = new CountDownLatch(1);
		pickerResultUri = null;

		runOnUiThread(() -> {
			Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
			intent.addCategory(Intent.CATEGORY_OPENABLE);
			intent.setType(mimeType);
			intent.putExtra(Intent.EXTRA_TITLE, suggestedName);
			startActivityForResult(intent, REQUEST_SAVE_FILE);
		});

		try { pickerLatch.await(); } catch (InterruptedException ignored) {}

		return (pickerResultUri != null) ? pickerResultUri : "";
	}

	/**
	 * Called from C++ after saving to a local file.
	 * Copies the local file content into the content:// URI obtained from pickFileForSave.
	 *
	 * @param localPath  absolute path to the locally saved file
	 * @param contentUri the content:// URI string returned by pickFileForSave
	 */
	public void writeFileToUri(String localPath, String contentUri)
	{
		try
		{
			File localFile = new File(localPath);
			if (!localFile.exists() || localFile.length() == 0)
			{
				System.err.println("writeFileToUri: local file missing or empty: " + localPath);
				return;
			}
			Uri uri = Uri.parse(contentUri);
			OutputStream out = getContentResolver().openOutputStream(uri);
			if (out == null)
			{
				System.err.println("writeFileToUri: openOutputStream returned null for: " + contentUri);
				return;
			}
			try (InputStream in = new FileInputStream(localFile); OutputStream o = out)
			{
				FileUtil.copyStream(in, o);
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		if ((requestCode == REQUEST_OPEN_FILE || requestCode == REQUEST_SAVE_FILE) && pickerLatch != null)
		{
			if (resultCode == Activity.RESULT_OK && data != null && data.getData() != null)
				pickerResultUri = data.getData().toString();
			else
				pickerResultUri = "";
			pickerLatch.countDown();
			return;
		}
		super.onActivityResult(requestCode, resultCode, data);
	}

}
