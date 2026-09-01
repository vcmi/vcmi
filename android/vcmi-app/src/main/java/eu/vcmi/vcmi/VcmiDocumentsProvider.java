package eu.vcmi.vcmi;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.webkit.MimeTypeMap;

import androidx.annotation.Nullable;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Locale;

/**
 * Publishes the vcmi data folder to the storage access framework.
 * Android hides app folders from the file picker, so without this the data folder can not be
 * reached with the system file manager or with any other app.
 */
public class VcmiDocumentsProvider extends DocumentsProvider
{
    private static final String AUTHORITY = BuildConfig.APPLICATION_ID + ".documents";
    private static final String ROOT_ID = "vcmi";

    private static final String[] DEFAULT_ROOT_PROJECTION = {
        Root.COLUMN_ROOT_ID,
        Root.COLUMN_FLAGS,
        Root.COLUMN_ICON,
        Root.COLUMN_TITLE,
        Root.COLUMN_SUMMARY,
        Root.COLUMN_DOCUMENT_ID,
        Root.COLUMN_AVAILABLE_BYTES,
    };

    private static final String[] DEFAULT_DOCUMENT_PROJECTION = {
        Document.COLUMN_DOCUMENT_ID,
        Document.COLUMN_MIME_TYPE,
        Document.COLUMN_DISPLAY_NAME,
        Document.COLUMN_LAST_MODIFIED,
        Document.COLUMN_FLAGS,
        Document.COLUMN_SIZE,
    };

    @Override
    public boolean onCreate()
    {
        return true;
    }

    @Override
    public Cursor queryRoots(final String[] projection) throws FileNotFoundException
    {
        final MatrixCursor result = new MatrixCursor(projection == null ? DEFAULT_ROOT_PROJECTION : projection);
        final File root = rootDir();

        // do not advertise a root before the game data folder exists
        if (!root.exists() && !root.mkdirs())
        {
            return result;
        }

        final MatrixCursor.RowBuilder row = result.newRow();
        put(result, row, Root.COLUMN_ROOT_ID, ROOT_ID);
        put(result, row, Root.COLUMN_DOCUMENT_ID, ROOT_ID);
        put(result, row, Root.COLUMN_TITLE, getContext().getString(R.string.data_folder_name));
        put(result, row, Root.COLUMN_SUMMARY, root.getAbsolutePath());
        put(result, row, Root.COLUMN_FLAGS, Root.FLAG_SUPPORTS_CREATE | Root.FLAG_SUPPORTS_IS_CHILD | Root.FLAG_LOCAL_ONLY);
        put(result, row, Root.COLUMN_ICON, R.mipmap.ic_launcher);
        put(result, row, Root.COLUMN_AVAILABLE_BYTES, root.getFreeSpace());

        return result;
    }

    @Override
    public Cursor queryDocument(final String documentId, final String[] projection) throws FileNotFoundException
    {
        final MatrixCursor result = new MatrixCursor(projection == null ? DEFAULT_DOCUMENT_PROJECTION : projection);
        addRow(result, idToFile(documentId), documentId);
        return result;
    }

    @Override
    public Cursor queryChildDocuments(final String parentDocumentId, final String[] projection, final String sortOrder) throws FileNotFoundException
    {
        final MatrixCursor result = new MatrixCursor(projection == null ? DEFAULT_DOCUMENT_PROJECTION : projection);
        final File[] children = idToFile(parentDocumentId).listFiles();

        if (children != null)
        {
            for (final File child : children)
            {
                addRow(result, child, null);
            }
        }

        // lets the picker refresh itself while vcmi or the user changes the folder
        result.setNotificationUri(getContext().getContentResolver(), DocumentsContract.buildChildDocumentsUri(AUTHORITY, parentDocumentId));

        return result;
    }

    @Override
    public ParcelFileDescriptor openDocument(final String documentId, final String mode, @Nullable final CancellationSignal signal) throws FileNotFoundException
    {
        return ParcelFileDescriptor.open(idToFile(documentId), ParcelFileDescriptor.parseMode(mode));
    }

    @Override
    public String createDocument(final String parentDocumentId, final String mimeType, final String displayName) throws FileNotFoundException
    {
        final File parent = idToFile(parentDocumentId);
        File target = new File(parent, displayName);

        for (int i = 1; target.exists(); ++i)
        {
            target = new File(parent, uniqueName(displayName, i));
        }

        try
        {
            final boolean created = Document.MIME_TYPE_DIR.equals(mimeType) ? target.mkdir() : target.createNewFile();
            if (!created)
            {
                throw new FileNotFoundException("Cannot create " + target.getAbsolutePath());
            }
        }
        catch (final IOException e)
        {
            throw new FileNotFoundException("Cannot create " + target.getAbsolutePath() + ": " + e);
        }

        notifyChange(parentDocumentId);

        return fileToId(target);
    }

    @Override
    public void deleteDocument(final String documentId) throws FileNotFoundException
    {
        final File file = idToFile(documentId);

        if (!delete(file))
        {
            throw new FileNotFoundException("Cannot delete " + file.getAbsolutePath());
        }

        notifyChange(parentId(documentId));
    }

    @Override
    public String renameDocument(final String documentId, final String displayName) throws FileNotFoundException
    {
        final File file = idToFile(documentId);
        final File target = new File(file.getParentFile(), displayName);

        if (target.exists() || !file.renameTo(target))
        {
            throw new FileNotFoundException("Cannot rename " + file.getAbsolutePath());
        }

        notifyChange(parentId(documentId));

        return fileToId(target);
    }

    @Override
    public boolean isChildDocument(final String parentDocumentId, final String documentId)
    {
        return documentId.startsWith(parentDocumentId + "/");
    }

    @Override
    public String getDocumentType(final String documentId) throws FileNotFoundException
    {
        return mimeType(idToFile(documentId));
    }

    private void addRow(final MatrixCursor result, final File file, @Nullable final String knownId) throws FileNotFoundException
    {
        final String documentId = knownId == null ? fileToId(file) : knownId;
        final boolean writable = file.canWrite();

        int flags = 0;
        if (writable)
        {
            flags |= file.isDirectory() ? Document.FLAG_DIR_SUPPORTS_CREATE : Document.FLAG_SUPPORTS_WRITE;

            // the data folder itself stays put - only its content may be removed or renamed
            if (!ROOT_ID.equals(documentId))
            {
                flags |= Document.FLAG_SUPPORTS_DELETE | Document.FLAG_SUPPORTS_RENAME;
            }
        }

        final MatrixCursor.RowBuilder row = result.newRow();
        put(result, row, Document.COLUMN_DOCUMENT_ID, documentId);
        put(result, row, Document.COLUMN_DISPLAY_NAME, file.getName());
        put(result, row, Document.COLUMN_MIME_TYPE, mimeType(file));
        put(result, row, Document.COLUMN_LAST_MODIFIED, file.lastModified());
        put(result, row, Document.COLUMN_FLAGS, flags);
        put(result, row, Document.COLUMN_SIZE, file.length());
    }

    /**
     * Callers may ask for a subset of the columns, and adding one they did not ask for throws.
     */
    private static void put(final MatrixCursor cursor, final MatrixCursor.RowBuilder row, final String column, final Object value)
    {
        if (cursor.getColumnIndex(column) >= 0)
        {
            row.add(column, value);
        }
    }

    private File rootDir() throws FileNotFoundException
    {
        if (getContext() == null || getContext().getExternalFilesDir(null) == null)
        {
            throw new FileNotFoundException("External storage is not available");
        }

        return Storage.getVcmiDataDir(getContext());
    }

    /**
     * Document ids are paths relative to the data folder, so they survive the folder being moved.
     */
    private File idToFile(final String documentId) throws FileNotFoundException
    {
        if (!ROOT_ID.equals(documentId) && !documentId.startsWith(ROOT_ID + "/"))
        {
            throw new FileNotFoundException("Unknown document " + documentId);
        }

        final File root = rootDir();
        final String relative = documentId.substring(ROOT_ID.length());
        final File file = relative.isEmpty() ? root : new File(root, relative);

        // ids are handed to us by other apps, so make sure they can not point outside of the data folder
        try
        {
            if (!file.getCanonicalPath().equals(root.getCanonicalPath())
                && !file.getCanonicalPath().startsWith(root.getCanonicalPath() + File.separator))
            {
                throw new FileNotFoundException("Document outside of the data folder: " + documentId);
            }
        }
        catch (final IOException e)
        {
            throw new FileNotFoundException("Cannot resolve " + documentId + ": " + e);
        }

        if (!file.exists())
        {
            throw new FileNotFoundException("No such document " + documentId);
        }

        return file;
    }

    private String fileToId(final File file) throws FileNotFoundException
    {
        final String root = rootDir().getAbsolutePath();
        final String path = file.getAbsolutePath();

        if (path.equals(root))
        {
            return ROOT_ID;
        }

        if (!path.startsWith(root + File.separator))
        {
            throw new FileNotFoundException("Document outside of the data folder: " + path);
        }

        return ROOT_ID + path.substring(root.length());
    }

    private static String parentId(final String documentId)
    {
        final int slash = documentId.lastIndexOf('/');
        return slash < 0 ? ROOT_ID : documentId.substring(0, slash);
    }

    private void notifyChange(final String documentId)
    {
        getContext().getContentResolver().notifyChange(DocumentsContract.buildChildDocumentsUri(AUTHORITY, documentId), null);
    }

    private static String uniqueName(final String displayName, final int index)
    {
        final int dot = displayName.lastIndexOf('.');

        if (dot <= 0)
        {
            return displayName + " (" + index + ")";
        }

        return displayName.substring(0, dot) + " (" + index + ")" + displayName.substring(dot);
    }

    private static String mimeType(final File file)
    {
        if (file.isDirectory())
        {
            return Document.MIME_TYPE_DIR;
        }

        final String name = file.getName();
        final int dot = name.lastIndexOf('.');

        if (dot >= 0)
        {
            final String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(name.substring(dot + 1).toLowerCase(Locale.ROOT));
            if (mime != null)
            {
                return mime;
            }
        }

        return "application/octet-stream";
    }

    private static boolean delete(final File file)
    {
        if (file.isDirectory())
        {
            final File[] children = file.listFiles();
            if (children != null)
            {
                for (final File child : children)
                {
                    delete(child);
                }
            }
        }

        return file.delete();
    }
}