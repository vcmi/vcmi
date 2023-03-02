package eu.vcmi.vcmi;

import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.style.UnderlineSpan;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import eu.vcmi.vcmi.content.DialogAuthors;
import eu.vcmi.vcmi.util.GeneratedVersion;
import eu.vcmi.vcmi.util.Utils;

/**
 * @author F
 */
public class ActivityAbout extends ActivityWithToolbar
{
    private static final String DIALOG_AUTHORS_TAG = "DIALOG_AUTHORS_TAG";

    @Override
    protected void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);
        initToolbar(R.string.about_title);

        initControl(R.id.about_version_app, getString(R.string.about_version_app, GeneratedVersion.VCMI_VERSION));
        initControl(R.id.about_version_launcher, getString(R.string.about_version_launcher, Utils.appVersionName(this)));
        initControlUrl(R.id.about_link_portal, R.string.about_links_main, R.string.url_project_page, this::onUrlPressed);
        initControlUrl(R.id.about_link_repo_main, R.string.about_links_repo, R.string.url_project_repo, this::onUrlPressed);
        initControlUrl(R.id.about_link_repo_launcher, R.string.about_links_repo_launcher, R.string.url_launcher_repo, this::onUrlPressed);
        initControlBtn(R.id.about_btn_authors, this::onBtnAuthorsPressed);
        initControlBtn(R.id.about_btn_libs, this::onBtnLibsPressed);
        initControlUrl(R.id.about_btn_privacy, R.string.about_btn_privacy, R.string.url_launcher_privacy, this::onUrlPressed);
    }

    private void initControlBtn(final int viewId, final View.OnClickListener callback)
    {
        findViewById(viewId).setOnClickListener(callback);
    }

    private void initControlUrl(final int textViewResId, final int baseTextRes, final int urlTextRes, final IInternalUrlCallback callback)
    {
        final TextView ctrl = (TextView) findViewById(textViewResId);
        final String urlText = getString(urlTextRes);
        final String fullText = getString(baseTextRes, urlText);

        ctrl.setText(decoratedLinkText(fullText, fullText.indexOf(urlText), fullText.length()));
        ctrl.setOnClickListener(v -> callback.onPressed(urlText));
    }

    private Spanned decoratedLinkText(final String rawText, final int start, final int end)
    {
        final SpannableString spannableString = new SpannableString(rawText);
        spannableString.setSpan(new UnderlineSpan(), start, end, 0);
        spannableString.setSpan(new ForegroundColorSpan(ContextCompat.getColor(this, R.color.accent)), start, end, 0);
        return spannableString;
    }

    private void initControl(final int textViewResId, final String text)
    {
        ((TextView) findViewById(textViewResId)).setText(text);
    }

    private void onBtnAuthorsPressed(final View v)
    {
        final DialogAuthors dialogAuthors = new DialogAuthors();
        dialogAuthors.show(getSupportFragmentManager(), DIALOG_AUTHORS_TAG);
    }

    private void onBtnLibsPressed(final View v)
    {
        // TODO 3rd party libs view (dialog?)
    }

    private void onUrlPressed(final String url)
    {
        try
        {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        }
        catch (final ActivityNotFoundException ignored)
        {
            Toast.makeText(this, R.string.about_error_opening_url, Toast.LENGTH_LONG).show();
        }
    }

    private interface IInternalUrlCallback
    {
        void onPressed(final String link);
    }
}
