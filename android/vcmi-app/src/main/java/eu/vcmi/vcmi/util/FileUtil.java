package eu.vcmi.vcmi.util;

import android.app.Activity;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.documentfile.provider.DocumentFile;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

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
}
