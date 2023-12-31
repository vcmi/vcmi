package eu.vcmi.vcmi;

import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.Nullable;
import android.view.View;
import android.widget.TextView;

/**
 * @author F
 */
public class ActivityError extends ActivityWithToolbar
{
    public static final String ARG_ERROR_MSG = "ActivityError.msg";

    @Override
    protected void onCreate(@Nullable final Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_error);
        initToolbar(R.string.launcher_title);

        final View btnTryAgain = findViewById(R.id.error_btn_try_again);
        btnTryAgain.setOnClickListener(new OnErrorRetryPressed());

        final Bundle extras = getIntent().getExtras();
        if (extras != null)
        {
            final String errorMessage = extras.getString(ARG_ERROR_MSG);
            final TextView errorMessageView = (TextView) findViewById(R.id.error_message);
            if (errorMessage != null)
            {
                errorMessageView.setText(errorMessage);
            }
        }
    }

    private class OnErrorRetryPressed implements View.OnClickListener
    {
        @Override
        public void onClick(final View v)
        {
            // basically restarts main activity
            startActivity(new Intent(ActivityError.this, ActivityLauncher.class));
            finish();
        }
    }
}
