package eu.vcmi.vcmi.settings;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Environment;
import android.provider.DocumentsContract;
import android.view.View;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import androidx.appcompat.app.AppCompatActivity;
import androidx.documentfile.provider.DocumentFile;
import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.Storage;
import eu.vcmi.vcmi.util.FileUtil;

public class ExportDataController extends LauncherSettingController<Void, Void>
{
    public static final int PICK_DIRECTORY_TO_EXPORT = 4;

    private String progress;

    public ExportDataController(final AppCompatActivity act)
    {
        super(act);
    }

    @Override
    protected String mainText()
    {
        return mActivity.getString(R.string.launcher_btn_export_title);
    }

    @Override
    protected String subText()
    {
        if (progress != null)
        {
            return progress;
        }

        return mActivity.getString(R.string.launcher_btn_export_description);
    }

    @Override
    public void onClick(final View v)
    {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);

        intent.putExtra(
            DocumentsContract.EXTRA_INITIAL_URI,
            Uri.fromFile(new File(Environment.getExternalStorageDirectory(), "vcmi-data")));

        mActivity.startActivityForResult(intent, PICK_DIRECTORY_TO_EXPORT);
    }

    public void copyData(Uri targetFolder)
    {
        AsyncCopyData copyTask = new AsyncCopyData(mActivity, targetFolder);

        copyTask.execute();
    }

    private class AsyncCopyData extends AsyncTask<String, String, Boolean>
    {
        private Activity owner;
        private Uri targetFolder;

        public AsyncCopyData(Activity owner, Uri targetFolder)
        {
            this.owner = owner;
            this.targetFolder = targetFolder;
        }

        @Override
        protected Boolean doInBackground(final String... params)
        {
            File targetDir = Storage.getVcmiDataDir(owner);
            DocumentFile sourceDir = DocumentFile.fromTreeUri(owner, targetFolder);

            return copyDirectory(targetDir, sourceDir);
        }

        @Override
        protected void onPostExecute(Boolean result)
        {
            super.onPostExecute(result);

            if (result)
            {
                ExportDataController.this.progress = null;
                ExportDataController.this.updateContent();
            }
        }

        @Override
        protected void onProgressUpdate(String... values)
        {
            ExportDataController.this.progress = values[0];
            ExportDataController.this.updateContent();
        }

        private boolean copyDirectory(File sourceDir, DocumentFile targetDir)
        {
            for (File child : sourceDir.listFiles())
            {
                DocumentFile exported = targetDir.findFile(child.getName());

                if (child.isFile())
                {
                    publishProgress(owner.getString(R.string.launcher_progress_copy,
                            child.getName()));

                    if (exported == null)
                    {
                        try
                        {
                            exported = targetDir.createFile(
                                "application/octet-stream",
                                child.getName());
                        }
                        catch (UnsupportedOperationException e)
                        {
                            publishProgress("Failed to copy file " + child.getName());

                            return false;
                        }
                    }

                    try(
                            final OutputStream targetStream = owner.getContentResolver()
                                    .openOutputStream(exported.getUri());
                            final InputStream sourceStream = new FileInputStream(child))
                    {
                        FileUtil.copyStream(sourceStream, targetStream);
                    }
                    catch (IOException e)
                    {
                        publishProgress("Failed to copy file " + child.getName());

                        return false;
                    }
                }

                if (child.isDirectory())
                {
                    if (exported == null)
                    {
                        exported = targetDir.createDirectory(child.getName());
                    }

                    if(!copyDirectory(child, exported))
                    {
                        return false;
                    }
                }
            }

            return true;
        }
    }
}
