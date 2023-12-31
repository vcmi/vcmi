package eu.vcmi.vcmi.mods;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

import eu.vcmi.vcmi.util.AsyncRequest;
import eu.vcmi.vcmi.util.Log;
import eu.vcmi.vcmi.util.ServerResponse;

/**
 * @author F
 */
public class VCMIModsRepo
{
    private final List<VCMIMod> mModsList;
    private IOnModsRepoDownloaded mCallback;

    public VCMIModsRepo()
    {
        mModsList = new ArrayList<>();
    }

    public void init(final String url, final IOnModsRepoDownloaded callback)
    {
        mCallback = callback;
        new AsyncLoadRepo().execute(url);
    }

    public interface IOnModsRepoDownloaded
    {
        void onSuccess(ServerResponse<List<VCMIMod>> response);
        void onError(final int code);
    }

    private class AsyncLoadRepo extends AsyncRequest<List<VCMIMod>>
    {
        @Override
        protected ServerResponse<List<VCMIMod>> doInBackground(final String... params)
        {
            ServerResponse<List<VCMIMod>> serverResponse = sendRequest(params[0]);
            if (serverResponse.isValid())
            {
                final List<VCMIMod> mods = new ArrayList<>();
                try
                {
                    JSONObject jsonContent = new JSONObject(serverResponse.mRawContent);
                    final JSONArray names = jsonContent.names();
                    for (int i = 0; i < names.length(); ++i)
                    {
                        try
                        {
                            String name = names.getString(i);
                            JSONObject modDownloadData = jsonContent.getJSONObject(name);

                            if(modDownloadData.has("mod"))
                            {
                                String modFileAddress = modDownloadData.getString("mod");
                                ServerResponse<List<VCMIMod>> modFile = sendRequest(modFileAddress);

                                if (!modFile.isValid())
                                {
                                    continue;
                                }

                                JSONObject modJson = new JSONObject(modFile.mRawContent);
                                mods.add(VCMIMod.buildFromRepoJson(name, modJson, modDownloadData));
                            }
                            else
                            {
                                mods.add(VCMIMod.buildFromRepoJson(name, modDownloadData, modDownloadData));
                            }
                        }
                        catch (JSONException e)
                        {
                            Log.e(this, "Could not parse the response as json", e);
                        }
                    }
                    serverResponse.mContent = mods;
                }
                catch (JSONException e)
                {
                    Log.e(this, "Could not parse the response as json", e);
                    serverResponse.mCode = ServerResponse.LOCAL_ERROR_PARSING;
                }
            }
            return serverResponse;
        }

        @Override
        protected void onPostExecute(final ServerResponse<List<VCMIMod>> response)
        {
            if (response.isValid())
            {
                mModsList.clear();
                mModsList.addAll(response.mContent);
                mCallback.onSuccess(response);
            }
            else
            {
                mCallback.onError(response.mCode);
            }
        }
    }
}
