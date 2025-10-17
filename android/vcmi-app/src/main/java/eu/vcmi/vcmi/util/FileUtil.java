package eu.vcmi.vcmi.util;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;

import androidx.annotation.Nullable;
import androidx.documentfile.provider.DocumentFile;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.Exception;
import java.util.List;

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
    private static void copyFileFromUri(String sourceFileUri, String destinationFile, Context context)
    {
        try
        {
            final InputStream inputStream = new FileInputStream(context.getContentResolver().openFileDescriptor(Uri.parse(sourceFileUri), "r").getFileDescriptor());
            final OutputStream outputStream = new FileOutputStream(new File(destinationFile));

            copyStream(inputStream, outputStream);
        }
        catch (IOException e)
        {
            Log.e("copyFileFromUri failed: ", e);
        }
    }

    @SuppressWarnings(Const.JNI_METHOD_SUPPRESS)
    private static String getFilenameFromUri(String sourceFileUri, Context context)
    {
        String fileName = "";
        try
        {
            ContentResolver contentResolver = context.getContentResolver();
            Cursor cursor = contentResolver.query(Uri.parse(sourceFileUri), null, null, null, null);

            if (cursor != null && cursor.moveToFirst()) {
                int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (nameIndex != -1) {
                    fileName = cursor.getString(nameIndex);
                }
                cursor.close();
            }
        }
        catch (Exception e)
        {
            Log.e("getFilenameFromUri failed: ", e);
        }

        return fileName;
    }

	// Build a flat list of copy tasks for a SAF tree selection.
	// Output rows are "srcUri\tTarget\tName" where Target is one of Data/Maps/Mp3 decided by file extension.
	// Heuristic: if user picked "Data" and we found no Maps/Mp3, also scan ../Maps and ../Mp3 one level up.
	public static String[] findFilesForCopy(String treeUriStr, android.content.Context ctx) {
	    // Build a DocumentFile tree from the SAF tree URI
	    final android.net.Uri treeUri = android.net.Uri.parse(treeUriStr);
	    final androidx.documentfile.provider.DocumentFile root =  androidx.documentfile.provider.DocumentFile.fromTreeUri(ctx, treeUri);
	    if (root == null) return new String[0];
	
	    final java.util.ArrayList<String> out = new java.util.ArrayList<>();
	    int foundMaps = 0, foundMp3 = 0;
	
	    // 1) Traverse the selected tree, collect files by extension
	    {
	        final java.util.ArrayDeque<androidx.documentfile.provider.DocumentFile> stack = new java.util.ArrayDeque<>();
	        stack.push(root);
	
	        while (!stack.isEmpty()) {
	            final androidx.documentfile.provider.DocumentFile d = stack.pop();
	            final androidx.documentfile.provider.DocumentFile[] kids = d.listFiles();
	            if (kids == null) continue;
	
	            for (androidx.documentfile.provider.DocumentFile f : kids) {
	                if (f.isDirectory()) { stack.push(f); continue; }
	                final String name = f.getName();
	                if (name == null) continue;
	                final String lower = name.toLowerCase(java.util.Locale.ROOT);
	
	                String target = null;
	                if (lower.endsWith(".lod") || lower.endsWith(".snd") || lower.endsWith(".vid") || lower.endsWith(".pak"))
	                    target = "Data";
	                else if (lower.endsWith(".h3m"))
	                    target = "Maps";
	                else if (lower.endsWith(".mp3"))
	                    target = "Mp3";
	
	                if (target != null) {
	                    out.add(f.getUri().toString() + "\t" + target + "\t" + name);
	                    if ("Maps".equals(target)) foundMaps++;
	                    if ("Mp3".equals(target))  foundMp3++;
	                }
	            }
	        }
	    }
	
	    // 2) If user picked the "Data" folder itself and Maps/Mp3 were not found, also scan siblings ../Maps and ../Mp3 one level up (heuristic).
	    try {
	        final String display = root.getName(); // tree top display name
	        if (display != null && display.equalsIgnoreCase("Data") && (foundMaps == 0 || foundMp3 == 0)) {
	            // Compute parent tree docId: trim last path segment of current treeDocId
	            final String treeDocId = android.provider.DocumentsContract.getTreeDocumentId(treeUri);
	            final int colon = (treeDocId != null) ? treeDocId.indexOf(':') : -1;
	            final String type = (colon < 0) ? treeDocId : treeDocId.substring(0, colon);
	            final String rel  = (colon < 0) ? ""         : treeDocId.substring(colon + 1);
	
	            final int slash = rel.lastIndexOf('/');
	            final String parentRel = (slash >= 0) ? rel.substring(0, slash) : "";
	            final String parentDocId = type + ":" + parentRel;
	
	            final android.net.Uri parentTree = android.provider.DocumentsContract.buildTreeDocumentUri(treeUri.getAuthority(), parentDocId);
	            final androidx.documentfile.provider.DocumentFile parentRoot = androidx.documentfile.provider.DocumentFile.fromTreeUri(ctx, parentTree);
	
	            if (parentRoot != null) {
	                // Helper to find a child folder by (case-insensitive) name under parentRoot
	                java.util.function.Function<String, androidx.documentfile.provider.DocumentFile> childByName =
	                        (folderName) -> {
	                            final androidx.documentfile.provider.DocumentFile[] kids = parentRoot.listFiles();
	                            if (kids == null) return null;
	                            for (androidx.documentfile.provider.DocumentFile f : kids) {
	                                if (f.isDirectory()) {
	                                    final String nm = f.getName();
	                                    if (nm != null && nm.equalsIgnoreCase(folderName)) return f;
	                                }
	                            }
	                            return null;
	                        };
	
	                // Append files from ../Maps (*.h3m)
	                if (foundMaps == 0) {
	                    final androidx.documentfile.provider.DocumentFile maps = childByName.apply("Maps");
	                    if (maps != null) {
	                        final java.util.ArrayDeque<androidx.documentfile.provider.DocumentFile> st = new java.util.ArrayDeque<>();
	                        st.push(maps);
	                        while (!st.isEmpty()) {
	                            final androidx.documentfile.provider.DocumentFile d = st.pop();
	                            final androidx.documentfile.provider.DocumentFile[] kids = d.listFiles();
	                            if (kids == null) continue;
	                            for (androidx.documentfile.provider.DocumentFile f : kids) {
	                                if (f.isDirectory()) { st.push(f); continue; }
	                                final String name = f.getName();
	                                if (name == null) continue;
	                                if (name.toLowerCase(java.util.Locale.ROOT).endsWith(".h3m")) {
	                                    out.add(f.getUri().toString() + "\tMaps\t" + name);
	                                    foundMaps++;
	                                }
	                            }
	                        }
	                    }
	                }
	
	                // Append files from ../Mp3 (*.mp3)
	                if (foundMp3 == 0) {
	                    final androidx.documentfile.provider.DocumentFile mp3 = childByName.apply("Mp3");
	                    if (mp3 != null) {
	                        final java.util.ArrayDeque<androidx.documentfile.provider.DocumentFile> st = new java.util.ArrayDeque<>();
	                        st.push(mp3);
	                        while (!st.isEmpty()) {
	                            final androidx.documentfile.provider.DocumentFile d = st.pop();
	                            final androidx.documentfile.provider.DocumentFile[] kids = d.listFiles();
	                            if (kids == null) continue;
	                            for (androidx.documentfile.provider.DocumentFile f : kids) {
	                                if (f.isDirectory()) { st.push(f); continue; }
	                                final String name = f.getName();
	                                if (name == null) continue;
	                                if (name.toLowerCase(java.util.Locale.ROOT).endsWith(".mp3")) {
	                                    out.add(f.getUri().toString() + "\tMp3\t" + name);
	                                    foundMp3++;
	                                }
	                            }
	                        }
	                    }
	                }
	            }
	        }
	    } catch (Throwable ignore) {
	        // If heuristic fails, just return what we already have
	    }
	
	    return out.toArray(new String[0]);
	}
	
}
