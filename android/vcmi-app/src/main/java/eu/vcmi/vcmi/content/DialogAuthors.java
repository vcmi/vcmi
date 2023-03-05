package eu.vcmi.vcmi.content;

import android.annotation.SuppressLint;
import android.app.Dialog;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.fragment.app.DialogFragment;
import androidx.appcompat.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import java.io.IOException;

import eu.vcmi.vcmi.R;
import eu.vcmi.vcmi.util.FileUtil;
import eu.vcmi.vcmi.util.Log;

/**
 * @author F
 */
public class DialogAuthors extends DialogFragment
{
    @NonNull
    @Override
    public Dialog onCreateDialog(final Bundle savedInstanceState)
    {
        final LayoutInflater inflater = LayoutInflater.from(getActivity());
        @SuppressLint("InflateParams") final View inflated = inflater.inflate(R.layout.dialog_authors, null, false);
        final TextView vcmiAuthorsView = (TextView) inflated.findViewById(R.id.dialog_authors_vcmi);
        final TextView launcherAuthorsView = (TextView) inflated.findViewById(R.id.dialog_authors_launcher);
        loadAuthorsContent(vcmiAuthorsView, launcherAuthorsView);
        return new AlertDialog.Builder(getActivity())
            .setView(inflated)
            .create();
    }

    private void loadAuthorsContent(final TextView vcmiAuthorsView, final TextView launcherAuthorsView)
    {
        try
        {
            // to be checked if this should be converted to async load (not really a file operation so it should be okay)
            final String authorsContent = FileUtil.read(getResources().openRawResource(R.raw.authors));
            vcmiAuthorsView.setText(authorsContent);
            launcherAuthorsView.setText("Fay"); // TODO hardcoded for now
        }
        catch (final IOException e)
        {
            Log.e(this, "Could not load authors content", e);
        }
    }
}
