package eu.vcmi.vcmi.util;

/**
 * @author F
 */
public class ServerResponse<T>
{
    public static final int LOCAL_ERROR_IO = -1;
    public static final int LOCAL_ERROR_PARSING = -2;

    public int mCode;
    public String mRawContent;
    public T mContent;

    public ServerResponse(final int code, final String content)
    {
        mCode = code;
        mRawContent = content;
    }

    public static boolean isResponseCodeValid(final int responseCode)
    {
        return responseCode >= 200 && responseCode < 300;
    }

    public boolean isValid()
    {
        return isResponseCodeValid(mCode);
    }
}
