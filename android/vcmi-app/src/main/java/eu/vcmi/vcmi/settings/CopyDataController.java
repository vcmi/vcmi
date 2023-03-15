package eu.vcmi.vcmi.settings;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.view.View;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

import androidx.appcompat.app.AppCompatActivity;
import androidx.documentfile.provider.DocumentFile;
import androidx.loader.content.AsyncTaskLoader;
import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.Storage;
import eu.vcmi.vcmi.util.FileUtil;

public class CopyDataController extends LauncherSettingController<Void, Void>
{
    public static final int PICK_EXTERNAL_VCMI_DATA_TO_COPY = 3;

    private String progress;

    public CopyDataController(final AppCompatActivity act)
    {
        super(act);
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_import_title);
    }

    @Override
    protected String subText()
    {
        if (progress != null)
        {
            return progress;
        }

        return mActivity.getString(R.string.launcher_btn_import_description);
    }

    @Override
    public void onClick(final View v)
    {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

        intent.putExtra(
                DocumentsContract.EXTRA_INITIAL_URI,
                Uri.fromFile(new File(Environment.getExternalStorageDirectory(), "vcmi-data")));

        mActivity.startActivityForResult(intent, PICK_EXTERNAL_VCMI_DATA_TO_COPY);
    }

    public void copyData(Uri folderToCopy)
    {
        AsyncCopyData copyTask = new AsyncCopyData(mActivity, folderToCopy);

        copyTask.execute();
    }

    private class AsyncCopyData extends AsyncTask<String, String, Boolean>
    {
        private Activity owner;
        private Uri folderToCopy;

        public AsyncCopyData(Activity owner, Uri folderToCopy)
        {
            this.owner = owner;
            this.folderToCopy = folderToCopy;
        }

        @Override
        protected Boolean doInBackground(final String... params)
        {
            File targetDir = Storage.getVcmiDataDir(owner);
            DocumentFile sourceDir = DocumentFile.fromTreeUri(owner, folderToCopy);

            ArrayList<String> allowedFolders = new ArrayList<String>();

            allowedFolders.add("Data");
            allowedFolders.add("Mp3");
            allowedFolders.add("Maps");
            allowedFolders.add("Saves");
            allowedFolders.add("Mods");
            allowedFolders.add("config");

            return copyDirectory(targetDir, sourceDir, allowedFolders);
        }

        @Override
        protected void onPostExecute(Boolean result)
        {
            super.onPostExecute(result);

            if (result)
            {
                CopyDataController.this.progress = null;
                CopyDataController.this.updateContent();
            }
        }

        @Override
        protected void onProgressUpdate(String... values)
        {
            CopyDataController.this.progress = values[0];
            CopyDataController.this.updateContent();
        }

        private boolean copyDirectory(File targetDir, DocumentFile sourceDir, List<String> allowed)
        {
            if (!targetDir.exists())
            {
                targetDir.mkdir();
            }

            for (DocumentFile child : sourceDir.listFiles())
            {
                if (allowed != null && !allowed.contains(child.getName()))
                {
                    continue;
                }

                File exported = new File(targetDir, child.getName());

                if (child.isFile())
                {
                    publishProgress(owner.getString(R.string.launcher_progress_copy,
                            child.getName()));

                    if (!exported.exists())
                    {
                        try
                        {
                            exported.createNewFile();
                        }
                        catch (IOException e)
                        {
                            publishProgress("Failed to copy file " + child.getName());

                            return false;
                        }
                    }

                    try (
                        final OutputStream targetStream = new FileOutputStream(exported, false);
                        final InputStream sourceStream = owner.getContentResolver()
                                .openInputStream(child.getUri()))
                    {
                        FileUtil.copyStream(sourceStream, targetStream);
                    }
                    catch (IOException e)
                    {
                        publishProgress("Failed to copy file " + child.getName());

                        return false;
                    }
                }

                if (child.isDirectory() && !copyDirectory(exported, child, null))
                {
                    return false;
                }
            }

            return true;
        }
    }
}

