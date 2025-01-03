package eu.vcmi.vcmi.util;

import android.app.Activity;
import android.content.Context;
import android.net.Uri;
import android.content.ContentResolver;
import android.provider.OpenableColumns;
import android.database.Cursor;

import androidx.annotation.Nullable;
import androidx.documentfile.provider.DocumentFile;

import org.libsdl.app.SDL;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;
import java.lang.Exception;

import eu.vcmi.vcmi.Const;
import eu.vcmi.vcmi.Storage;

/**
 * @author F
 */
public class FileUtil
{
    private static final int BUFFER_SIZE = 4096;

    public static boolean copyData(Uri folderToCopy, Activity activity)
    {
        File targetDir = Storage.getVcmiDataDir(activity);
        DocumentFile sourceDir = DocumentFile.fromTreeUri(activity, folderToCopy);
        return copyDirectory(targetDir, sourceDir, List.of("Data", "Maps", "Mp3"), activity);
    }

    private static boolean copyDirectory(File targetDir, DocumentFile sourceDir, @Nullable List<String> allowed, Activity activity)
    {
        if (!targetDir.exists())
        {
            targetDir.mkdir();
        }

        for (DocumentFile child : sourceDir.listFiles())
        {
            if (allowed != null)
            {
                boolean fileAllowed = false;

                for (String str : allowed)
                {
                    if (str.equalsIgnoreCase(child.getName()))
                    {
                        fileAllowed = true;
                        break;
                    }
                }

                if (!fileAllowed)
                    continue;
            }

            File exported = new File(targetDir, child.getName());

            if (child.isFile())
            {
                if (!exported.exists())
                {
                    try
                    {
                        exported.createNewFile();
                    }
                    catch (IOException e)
                    {
                        Log.e(activity, "createNewFile failed: " + e);
                        return false;
                    }
                }

                try (
                    final OutputStream targetStream = new FileOutputStream(exported, false);
                    final InputStream sourceStream = activity.getContentResolver()
                            .openInputStream(child.getUri()))
                {
                    copyStream(sourceStream, targetStream);
                }
                catch (IOException e)
                {
                    Log.e(activity, "copyStream failed: " + e);
                    return false;
                }
            }

            if (child.isDirectory() && !copyDirectory(exported, child, null, activity))
            {
                return false;
            }
        }

        return true;
    }

    private static void copyStream(InputStream source, OutputStream target) throws IOException
    {
        final byte[] buffer = new byte[BUFFER_SIZE];
        int read;
        while ((read = source.read(buffer)) != -1)
        {
            target.write(buffer, 0, read);
        }
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    private static void copyFileFromUri(String sourceFileUri, String destinationFile)
    {
        try
        {
            final Context ctx = SDL.getContext();
            final InputStream inputStream = new FileInputStream(ctx.getContentResolver().openFileDescriptor(Uri.parse(sourceFileUri), "r").getFileDescriptor());
            final OutputStream outputStream = new FileOutputStream(new File(destinationFile));

            copyStream(inputStream, outputStream);
        }
        catch (IOException e)
        {
            Log.e("copyFileFromUri failed: ", e);
        }
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    private static String getFilenameFromUri(String sourceFileUri)
    {
        try
        {
            String fileName = "";

            final Context ctx = SDL.getContext();
            ContentResolver contentResolver = ctx.getContentResolver();
            Cursor cursor = contentResolver.query(Uri.parse(sourceFileUri), null, null, null, null);

            if (cursor != null && cursor.moveToFirst()) {
                int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (nameIndex != -1) {
                    fileName = cursor.getString(nameIndex);
                }
                cursor.close();
            }

            return fileName;
        }
        catch (Exception e)
        {
            Log.e("getFilenameFromUri failed: ", e);

            return "";
        }
    }
}
