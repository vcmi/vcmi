package eu.vcmi.vcmi.util;

import android.annotation.TargetApi;
import android.os.AsyncTask;
import android.os.Build;
import androidx.annotation.RequiresApi;

import java.net.HttpURLConnection;
import java.net.URL;
import java.util.Scanner;

import eu.vcmi.vcmi.Const;

/**
 * @author F
 */
public abstract class AsyncRequest<T> extends AsyncTask<String, Void, ServerResponse<T>>
{
    @TargetApi(Const.SUPPRESS_TRY_WITH_RESOURCES_WARNING)
    protected ServerResponse<T> sendRequest(final String url)
    {

        try
        {
            final HttpURLConnection conn = (HttpURLConnection) new URL(url).openConnection();
            final int responseCode = conn.getResponseCode();
            if (!ServerResponse.isResponseCodeValid(responseCode))
            {
                return new ServerResponse<>(responseCode, null);
            }

            try (Scanner s = new java.util.Scanner(conn.getInputStream()).useDelimiter("\\A"))
            {
                final String response = s.hasNext() ? s.next() : "";
                return new ServerResponse<>(responseCode, response);
            }
            catch (final Exception e)
            {
                Log.e(this, "Request failed: ", e);
            }
        }
        catch (final Exception e)
        {
            Log.e(this, "Request failed: ", e);
        }
        return new ServerResponse<>(ServerResponse.LOCAL_ERROR_IO, null);
    }

}
