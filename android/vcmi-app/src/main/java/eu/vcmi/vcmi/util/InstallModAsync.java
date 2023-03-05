package eu.vcmi.vcmi.util;

import android.content.Context;
import android.nfc.FormatException;
import android.os.AsyncTask;
import android.os.Build;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileFilter;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;

public class InstallModAsync
        extends AsyncTask<String, String, Boolean>
        implements IZipProgressReporter
{
    private static final String TAG = "DOWNLOADFILE";
    private static final int DOWNLOAD_PERCENT = 70;

    private PostDownload callback;
    private File downloadLocation;
    private File extractLocation;
    private Context context;
    private int totalFiles;
    private int unpackedFiles;

    public InstallModAsync(File extractLocation, Context context, PostDownload callback)
    {
        this.context = context;
        this.callback = callback;
        this.extractLocation = extractLocation;
    }

    @Override
    protected Boolean doInBackground(String... args)
    {
        int count;

        try
        {
            File modsFolder = extractLocation.getParentFile();

            if (!modsFolder.exists()) modsFolder.mkdir();

            this.downloadLocation = File.createTempFile("tmp", ".zip", modsFolder);

            URL url = new URL(args[0]);
            URLConnection connection = url.openConnection();
            connection.connect();

            long lengthOfFile = -1;

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            {
                lengthOfFile = connection.getContentLengthLong();
            }

            if(lengthOfFile == -1)
            {
                try
                {
                    lengthOfFile = Long.parseLong(connection.getHeaderField("Content-Length"));
                    Log.d(TAG, "Length of the file: " + lengthOfFile);
                } catch (NumberFormatException ex)
                {
                    Log.d(TAG, "Failed to parse content length", ex);
                }
            }

            if(lengthOfFile == -1)
            {
                lengthOfFile = 100000000;
                Log.d(TAG, "Using dummy length of file");
            }

            InputStream input = new BufferedInputStream(url.openStream());
            FileOutputStream output = new FileOutputStream(downloadLocation); //context.openFileOutput("content.zip", Context.MODE_PRIVATE);
            Log.d(TAG, "file saved at " + downloadLocation.getAbsolutePath());

            byte data[] = new byte[1024];
            long total = 0;
            while ((count = input.read(data)) != -1)
            {
                total += count;
                output.write(data, 0, count);
                this.publishProgress((int) ((total * DOWNLOAD_PERCENT) / lengthOfFile) + "%");
            }

            output.flush();
            output.close();
            input.close();

            File tempDir = File.createTempFile("tmp", "", modsFolder);

            tempDir.delete();
            tempDir.mkdir();

            if (!extractLocation.exists()) extractLocation.mkdir();

            try
            {
                totalFiles = FileUtil.countFilesInZip(downloadLocation);
                unpackedFiles = 0;

                FileUtil.unpackZipFile(downloadLocation, tempDir, this);

                return moveModToExtractLocation(tempDir);
            }
            finally
            {
                downloadLocation.delete();
                FileUtil.clearDirectory(tempDir);
                tempDir.delete();
            }
        } catch (Exception e)
        {
            Log.e(TAG, "Unhandled exception while installing mod", e);
        }

        return false;
    }

    @Override
    protected void onProgressUpdate(String... values)
    {
        callback.downloadProgress(values);
    }

    @Override
    protected void onPostExecute(Boolean result)
    {
        if (callback != null) callback.downloadDone(result, extractLocation);
    }

    private boolean moveModToExtractLocation(File tempDir)
    {
        return moveModToExtractLocation(tempDir, 0);
    }

    private boolean moveModToExtractLocation(File tempDir, int level)
    {
        File[] modJson = tempDir.listFiles(new FileFilter()
        {
            @Override
            public boolean accept(File file)
            {
                return file.getName().equalsIgnoreCase("Mod.json");
            }
        });

        if (modJson != null && modJson.length > 0)
        {
            File modFolder = modJson[0].getParentFile();

            if (!modFolder.renameTo(extractLocation))
            {
                FileUtil.copyDir(modFolder, extractLocation);
            }

            return true;
        }

        if (level <= 1)
        {
            for (File child : tempDir.listFiles())
            {
                if (child.isDirectory() && moveModToExtractLocation(child, level + 1))
                {
                    return true;
                }
            }
        }

        return false;
    }

    @Override
    public void onUnpacked(File newFile)
    {
        unpackedFiles++;

        int progress = DOWNLOAD_PERCENT
                + (unpackedFiles * (100 - DOWNLOAD_PERCENT) / totalFiles);

        publishProgress(progress + "%");
    }

    public interface PostDownload
    {
        void downloadDone(Boolean succeed, File modFolder);

        void downloadProgress(String... progress);
    }
}