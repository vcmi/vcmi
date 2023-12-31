package eu.vcmi.vcmi.util;

import java.io.File;

public interface IZipProgressReporter
{
    void onUnpacked(File newFile);
}
