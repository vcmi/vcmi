package eu.vcmi.vcmi.util;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import android.provider.DocumentsContract;

import androidx.annotation.Nullable;
import androidx.documentfile.provider.DocumentFile;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.Exception;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.Locale;

import eu.vcmi.vcmi.Const;
import eu.vcmi.vcmi.Storage;

/**
 * @author F
 */
public class FileUtil
{
	private static final int BUFFER_SIZE = 64 * 1024;

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
		try (InputStream inputStream  = context.getContentResolver().openInputStream(Uri.parse(sourceFileUri));
			 FileOutputStream outputStream = new FileOutputStream(new File(destinationFile))) {
	            copyStream(inputStream, outputStream);
		        // ensure data is flushed and durably written
	            outputStream.flush();
	            outputStream.getFD().sync();
	        }

	    catch (IOException e)
		{
	        Log.e("FileUtil", "copyFileFromUri failed: " + sourceFileUri + " -> " + destinationFile, e);
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


	private static final String TARGET_DATA = "Data";
	private static final String TARGET_MAPS = "Maps";
	private static final String TARGET_MP3  = "Mp3";

/**
 * Build a flat list of copy tasks for a user-selected folder
 * Storage Access Framework (SAF) — i.e. a "SAF tree" from ACTION_OPEN_DOCUMENT_TREE.
 *
 * Output lines: "<uri>\t<Target>\t<Name>", where Target is {Data, Maps, Mp3}
 * decided by file extension:
 *  - Data:  .lod .snd .vid .pak
 *  - Maps:  .h3m
 *  - Mp3:   .mp3
 *
 * Heuristic: if user picked the "Data" folder and no Maps/Mp3 were found inside it,
 * also scan sibling folders ../Maps and ../Mp3 (one level up).
 */
public static String[] findFilesForCopy(String treeUriStr, Context ctx)
{
	final Uri treeUri = Uri.parse(treeUriStr);
	final DocumentFile root = DocumentFile.fromTreeUri(ctx, treeUri);
	
	if (root == null)
		return new String[0];

	final List<String> out = new ArrayList<>();
	int foundMaps = 0;
	int foundMp3  = 0;

	// 1) Traverse the selected tree, collect files by extension
	final Deque<DocumentFile> stack = new ArrayDeque<>();
	stack.push(root);

	while (!stack.isEmpty())
	{
		final DocumentFile entry = stack.pop();
		if (entry == null)
			continue;

		if (entry.isDirectory())
		{
			final DocumentFile[] children = entry.listFiles();
			if (children == null)
				continue;

			for (final DocumentFile child : children) {
				if (child != null)
					stack.push(child);
			}
			continue;
		}

		final String name = entry.getName();
		if (name == null)
			continue;

		final String lower = name.toLowerCase(Locale.ROOT);
		String target = null;

		if (lower.endsWith(".lod") || lower.endsWith(".snd") || lower.endsWith(".vid") || lower.endsWith(".pak"))
		{
			target = TARGET_DATA;
		}
		else if (lower.endsWith(".h3m"))
		{
			target = TARGET_MAPS;
			foundMaps++;
		}
		else if (lower.endsWith(".mp3"))
		{
			target = TARGET_MP3;
			foundMp3++;
		}

		if (target != null)
			out.add(String.format(Locale.ROOT, "%s\t%s\t%s", entry.getUri().toString(), target, name));

	}

	// 2) If user picked "Data" and Maps/Mp3 were not found, also scan ../Maps and ../Mp3
	final String display = root.getName();
	if (display != null && display.equalsIgnoreCase(TARGET_DATA) && (foundMaps == 0 || foundMp3 == 0))
	{
		final DocumentFile parentRoot = tryResolveParentOfTree(ctx, treeUri);
		if (parentRoot != null)
		{
			if (foundMaps == 0)
			{
				final DocumentFile maps = findChildDirIgnoreCase(parentRoot, TARGET_MAPS);
				if (maps != null)
					foundMaps += collectFilesByExt(maps, ".h3m", TARGET_MAPS, out);
			}
			if (foundMp3 == 0)
			{
				final DocumentFile mp3 = findChildDirIgnoreCase(parentRoot, TARGET_MP3);
				if (mp3 != null)
					foundMp3 += collectFilesByExt(mp3, ".mp3", TARGET_MP3, out);
			}
		}
	}

	return out.toArray(new String[0]);
}


private static int collectFilesByExt(final DocumentFile start, final String extLower, final String target, final List<String> out)
{
	int count = 0;
	final Deque<DocumentFile> stack = new ArrayDeque<>();
	stack.push(start);

	while (!stack.isEmpty())
	{
		final DocumentFile entry = stack.pop();
		if (entry == null)
			continue;

		if (entry.isDirectory())
		{
			final DocumentFile[] children = entry.listFiles();
			if (children == null)
				continue;

			for (final DocumentFile child : children)
			{
				if (child != null)
					stack.push(child);
			}
			continue;
		}

		final String name = entry.getName();
		if (name == null)
			continue;

		if (name.toLowerCase(Locale.ROOT).endsWith(extLower))
		{
			out.add(String.format(Locale.ROOT, "%s\t%s\t%s", entry.getUri().toString(), target, name));
			count++;
		}
	}
	return count;
}

@Nullable
private static DocumentFile findChildDirIgnoreCase(final DocumentFile parent, final String childName)
{
	final DocumentFile[] children = parent.listFiles();
	if (children == null)
		return null;

	for (final DocumentFile child : children)
	{
		if (child != null && child.isDirectory())
		{
			final String name = child.getName();
			if (name != null && name.equalsIgnoreCase(childName))
				return child;
		}
	}
	return null;
}

/**
 * Tries to resolve the parent folder of the current SAF tree.
 * Returns null if the tree is already at top-level (no parent).
 */
@Nullable
private static DocumentFile tryResolveParentOfTree(final Context ctx, final Uri treeUri) {
	try {
		final String treeDocId = DocumentsContract.getTreeDocumentId(treeUri);
		if (treeDocId == null)
			return null;

		final String[] parts = treeDocId.split(":", 2);
		final String type = parts.length > 0 ? parts[0] : "";
		final String rel  = parts.length > 1 ? parts[1] : "";

		final int slash = rel.lastIndexOf('/');
		if (slash < 0) {
			return null; // already at the top of the type root
		}

		final String parentRel = rel.substring(0, slash);
		final String parentDocId = type + ":" + parentRel;

		final Uri parentTree = DocumentsContract.buildTreeDocumentUri(treeUri.getAuthority(), parentDocId);

		return DocumentFile.fromTreeUri(ctx, parentTree);
	} catch (Exception e) {
		return null;
	}
}

}
