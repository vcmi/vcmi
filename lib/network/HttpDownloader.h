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

using HttpDownloadID = uint32_t;

/// Receives results of downloads performed by HttpDownloader
class DLL_LINKAGE IHttpDownloaderListener
{
public:
	virtual ~IHttpDownloaderListener() = default;

	/// Receives number of downloaded bytes and total size of the file, or 0 if server did not report it
	virtual void onDownloadProgress(HttpDownloadID download, uint64_t received, uint64_t total) = 0;
	/// Receives empty string on success, or description of the error
	/// certificateError: download failed because certificate of the server could not be verified
	virtual void onDownloadFinished(HttpDownloadID download, const std::string & errorMessage, bool certificateError) = 0;
};

/// Downloads files over HTTP(S) using libcurl. Downloads run in parallel, driven by periodic calls to poll()
class DLL_LINKAGE HttpDownloader : boost::noncopyable
{
public:
	/// listener: receives results of all downloads, must outlive downloader
	/// proxy: proxy URL in libcurl format, e.g. "socks5h://host:port". If empty, proxy environment variables are used
	HttpDownloader(IHttpDownloaderListener & listener, const std::string & proxy, bool ignoreSslErrors);
	~HttpDownloader();

	/// Starts download of url into target file. Returned identifier is passed to listener
	HttpDownloadID start(const std::string & url, const boost::filesystem::path & target);

	/// Advances active downloads without blocking. Listener is called from this method
	void poll();

	/// Aborts all active downloads and removes partially downloaded files. Listener is not called
	void cancel();

	bool isActive() const;

private:
	void finish(HttpDownloadID download, const std::string & errorMessage, bool certificateError);

	IHttpDownloaderListener & listener;
	std::map<HttpDownloadID, std::unique_ptr<HttpDownloaderTransfer>> transfers;
	std::vector<HttpDownloadID> failedToStart; ///< downloads that failed before reaching libcurl, reported on next poll()
	HttpDownloadID nextDownloadID = 0;
	void * multiHandle; ///< CURLM
	std::string proxy;
	std::string caCertificates; ///< PEM certificates provided by the system, on platforms where libcurl cannot find them
	std::string caCertificatesPath; ///< CA bundle file of the system, on platforms where libcurl cannot find it
	bool ignoreSslErrors;
};

#endif
