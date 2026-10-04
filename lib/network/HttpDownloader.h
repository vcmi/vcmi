/*
 * HttpDownloader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#ifdef ENABLE_LIBCURL

struct HttpDownloaderTransfer;

/// Receives results of downloads performed by HttpDownloader
class DLL_LINKAGE IHttpDownloaderListener
{
public:
	virtual ~IHttpDownloaderListener() = default;

	/// Receives number of downloaded bytes and total size of the file, or 0 if server did not report it
	virtual void onDownloadProgress(uint64_t received, uint64_t total) = 0;
	/// Receives empty string on success, or description of the error
	virtual void onDownloadFinished(const std::string & errorMessage) = 0;
};

/// Downloads files over HTTP(S) using libcurl. Performs one download at a time, driven by periodic calls to poll()
class DLL_LINKAGE HttpDownloader : boost::noncopyable
{
public:
	/// listener: receives results of all downloads, must outlive downloader
	/// proxy: proxy URL in libcurl format, e.g. "socks5h://host:port". If empty, proxy environment variables are used
	HttpDownloader(IHttpDownloaderListener & listener, const std::string & proxy, bool ignoreSslErrors);
	~HttpDownloader();

	/// Starts download of url into target file. Only one download can be active at a time
	void start(const std::string & url, const boost::filesystem::path & target);

	/// Advances active download without blocking. Listener is called from this method
	void poll();

	/// Aborts active download and removes partially downloaded file. Listener is not called
	void cancel();

	bool isActive() const;

private:
	void finish(const std::string & errorMessage);

	IHttpDownloaderListener & listener;
	std::unique_ptr<HttpDownloaderTransfer> transfer;
	void * multiHandle; ///< CURLM
	std::string proxy;
	std::string caCertificates; ///< PEM certificates provided by the system, on platforms where libcurl cannot find them
	std::string caCertificatesPath; ///< CA bundle file of the system, on platforms where libcurl cannot find it
	bool ignoreSslErrors;
};

#endif
