/*
 * HttpDownloader.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "HttpDownloader.h"

#ifdef ENABLE_LIBCURL

#include "../../Version.h"
#include "../texts/TextOperations.h"

#ifdef VCMI_ANDROID
#include "../CAndroidVMHelper.h"
#endif

#include <curl/curl.h>

/// State of a single download performed by HttpDownloader
struct HttpDownloaderTransfer
{
	HttpDownloaderTransfer(IHttpDownloaderListener & listener, void * multiHandle, HttpDownloadID id)
		: listener(listener)
		, multiHandle(multiHandle)
		, id(id)
	{}

	~HttpDownloaderTransfer()
	{
		if(handle)
		{
			// no-op if handle was never added to multi handle
			curl_multi_remove_handle(multiHandle, handle);
			curl_easy_cleanup(handle);
		}
	}

	IHttpDownloaderListener & listener;
	void * multiHandle;
	HttpDownloadID id;
	CURL * handle = nullptr;
	std::ofstream file;
	boost::filesystem::path target;
	std::string url;
	std::array<char, CURL_ERROR_SIZE> errorBuffer = {};
	std::string error; ///< error detected outside of libcurl, takes priority over error reported by libcurl
	curl_off_t reportedBytes = -1;

	static size_t writeCallback(char * data, size_t size, size_t count, void * userdata)
	{
		auto * transfer = static_cast<HttpDownloaderTransfer *>(userdata);
		transfer->file.write(data, size * count);
		if(!transfer->file)
		{
			transfer->error = "Failed to write file " + TextOperations::filesystemPathToUtf8(transfer->target);
			return 0;
		}
		return size * count;
	}

	static int progressCallback(void * userdata, curl_off_t totalBytes, curl_off_t receivedBytes, curl_off_t, curl_off_t)
	{
		auto * transfer = static_cast<HttpDownloaderTransfer *>(userdata);
		if(receivedBytes != transfer->reportedBytes)
		{
			transfer->reportedBytes = receivedBytes;
			transfer->listener.onDownloadProgress(transfer->id, receivedBytes, totalBytes);
		}
		return 0;
	}

	void removeTarget() const
	{
		boost::system::error_code ec;
		boost::filesystem::remove(target, ec);
		if(ec)
			logNetwork->warn("Failed to remove partially downloaded file %s: %s", TextOperations::filesystemPathToUtf8(target), ec.message());
	}
};

template<typename T>
static void setOption(CURL * handle, CURLoption option, T value)
{
	CURLcode result = curl_easy_setopt(handle, option, value);
	if(result != CURLE_OK)
		throw std::runtime_error("Failed to set libcurl option " + std::to_string(option) + ": " + curl_easy_strerror(result));
}

HttpDownloader::HttpDownloader(IHttpDownloaderListener & listener, const std::string & proxy, bool ignoreSslErrors)
	: listener(listener)
	, multiHandle(nullptr)
	, proxy(proxy)
	, ignoreSslErrors(ignoreSslErrors)
{
	CURLcode initResult = curl_global_init(CURL_GLOBAL_DEFAULT);
	if(initResult != CURLE_OK)
		throw std::runtime_error(std::string("Failed to initialize libcurl: ") + curl_easy_strerror(initResult));

	multiHandle = curl_multi_init();
	if(!multiHandle)
	{
		curl_global_cleanup();
		throw std::runtime_error("Failed to initialize libcurl multi handle");
	}

#if defined(VCMI_ANDROID)
	// OpenSSL can't read Android certificate store, so it is provided by Java code
	CAndroidVMHelper envHelper;
	caCertificates = envHelper.callStaticStringMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "getSystemCACertificates");
#elif defined(VCMI_XDG)
	// CA bundle path built into libcurl belongs to the build machine, which may differ from the system we run on (e.g. in AppImage)
	static const std::array<const char *, 5> caBundleLocations = {
		"/etc/ssl/certs/ca-certificates.crt", // Debian, Ubuntu, Arch, Gentoo
		"/etc/pki/tls/certs/ca-bundle.crt", // Fedora, RHEL
		"/etc/ssl/ca-bundle.pem", // openSUSE
		"/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem", // CentOS
		"/etc/ssl/cert.pem", // Alpine, BSD
	};

	for(const auto * location : caBundleLocations)
	{
		if(boost::filesystem::exists(location))
		{
			caCertificatesPath = location;
			break;
		}
	}

	if(caCertificatesPath.empty())
		logNetwork->warn("CA certificates bundle not found, using libcurl defaults");
#endif
}

HttpDownloader::~HttpDownloader()
{
	cancel();
	curl_multi_cleanup(multiHandle);
	curl_global_cleanup();
}

HttpDownloadID HttpDownloader::start(const std::string & url, const boost::filesystem::path & target)
{
	const HttpDownloadID id = nextDownloadID++;
	auto newTransfer = std::make_unique<HttpDownloaderTransfer>(listener, multiHandle, id);
	newTransfer->url = url;
	newTransfer->target = target;

	newTransfer->handle = curl_easy_init();
	if(!newTransfer->handle)
		throw std::runtime_error("Failed to initialize libcurl handle");

	CURL * handle = newTransfer->handle;
	const std::string userAgent = std::string("VCMI/") + GameConstants::VCMI_VERSION;

	setOption(handle, CURLOPT_URL, url.c_str());
	setOption(handle, CURLOPT_PRIVATE, newTransfer.get());
	setOption(handle, CURLOPT_USERAGENT, userAgent.c_str());
	setOption(handle, CURLOPT_ERRORBUFFER, newTransfer->errorBuffer.data());
	setOption(handle, CURLOPT_FAILONERROR, 1L);
	setOption(handle, CURLOPT_NOSIGNAL, 1L);
	setOption(handle, CURLOPT_FOLLOWLOCATION, 1L);
	setOption(handle, CURLOPT_MAXREDIRS, 10L);
	// redirects may only lead to encrypted connections
#if LIBCURL_VERSION_NUM >= 0x075500
	setOption(handle, CURLOPT_PROTOCOLS_STR, "http,https");
	setOption(handle, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
	setOption(handle, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS));
	setOption(handle, CURLOPT_REDIR_PROTOCOLS, static_cast<long>(CURLPROTO_HTTPS));
#endif
	// TLS 1.2 is disabled by default on Windows 7 and must be requested explicitly
	setOption(handle, CURLOPT_SSLVERSION, static_cast<long>(CURL_SSLVERSION_TLSv1_2));
	// revocation servers are often unreachable behind firewalls, which would make Schannel reject the connection
	setOption(handle, CURLOPT_SSL_OPTIONS, static_cast<long>(CURLSSLOPT_REVOKE_BEST_EFFORT));
	setOption(handle, CURLOPT_WRITEFUNCTION, &HttpDownloaderTransfer::writeCallback);
	setOption(handle, CURLOPT_WRITEDATA, newTransfer.get());
	setOption(handle, CURLOPT_XFERINFOFUNCTION, &HttpDownloaderTransfer::progressCallback);
	setOption(handle, CURLOPT_XFERINFODATA, newTransfer.get());
	setOption(handle, CURLOPT_NOPROGRESS, 0L);

	if(!proxy.empty())
		setOption(handle, CURLOPT_PROXY, proxy.c_str());

	if(ignoreSslErrors)
	{
		setOption(handle, CURLOPT_SSL_VERIFYPEER, 0L);
		setOption(handle, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	if(!caCertificates.empty())
	{
		curl_blob blob = {caCertificates.data(), caCertificates.size(), CURL_BLOB_COPY};
		setOption(handle, CURLOPT_CAINFO_BLOB, &blob);
	}

	if(!caCertificatesPath.empty())
		setOption(handle, CURLOPT_CAINFO, caCertificatesPath.c_str());

	newTransfer->file.open(target.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	if(!newTransfer->file)
	{
		// reported on next poll(), same as any other failure
		newTransfer->error = "Failed to open file " + TextOperations::filesystemPathToUtf8(target);
		transfers[id] = std::move(newTransfer);
		failedToStart.push_back(id);
		return id;
	}

	CURLMcode addResult = curl_multi_add_handle(multiHandle, handle);
	if(addResult != CURLM_OK)
	{
		newTransfer->file.close();
		newTransfer->removeTarget();
		throw std::runtime_error(std::string("Failed to start libcurl transfer: ") + curl_multi_strerror(addResult));
	}

	transfers[id] = std::move(newTransfer);
	return id;
}

void HttpDownloader::poll()
{
	// listener may start or cancel downloads, so collections are not iterated while it is called
	for(HttpDownloadID id : std::exchange(failedToStart, {}))
	{
		if(transfers.count(id))
			finish(id, transfers.at(id)->error);
	}

	if(transfers.empty())
		return;

	int runningTransfers = 0;
	CURLMcode performResult = curl_multi_perform(multiHandle, &runningTransfers);
	if(performResult != CURLM_OK)
		throw std::runtime_error(std::string("Failed to perform libcurl transfers: ") + curl_multi_strerror(performResult));

	int queuedMessages = 0;
	// messages of downloads removed by listener are discarded by libcurl
	while(CURLMsg * message = curl_multi_info_read(multiHandle, &queuedMessages))
	{
		if(message->msg != CURLMSG_DONE)
			continue;

		HttpDownloaderTransfer * transfer = nullptr;
		curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &transfer);

		CURLcode result = message->data.result;
		if(result == CURLE_OK)
			finish(transfer->id, {});
		else if(!transfer->error.empty())
			finish(transfer->id, "Failed to download " + transfer->url + ": " + transfer->error);
		else if(transfer->errorBuffer.front() != '\0')
			finish(transfer->id, "Failed to download " + transfer->url + ": " + transfer->errorBuffer.data());
		else
			finish(transfer->id, "Failed to download " + transfer->url + ": " + curl_easy_strerror(result));
	}
}

void HttpDownloader::finish(HttpDownloadID download, const std::string & errorMessage)
{
	std::unique_ptr<HttpDownloaderTransfer> finished = std::move(transfers.at(download));
	transfers.erase(download);
	std::string result = errorMessage;

	finished->file.close();
	if(result.empty() && !finished->file)
		result = "Failed to write file " + TextOperations::filesystemPathToUtf8(finished->target);

	if(!result.empty())
		finished->removeTarget();

	finished.reset();
	listener.onDownloadFinished(download, result);
}

void HttpDownloader::cancel()
{
	auto cancelled = std::move(transfers);
	transfers.clear();
	failedToStart.clear();

	for(auto & [id, transfer] : cancelled)
	{
		transfer->file.close();
		transfer->removeTarget();
	}
}

bool HttpDownloader::isActive() const
{
	return !transfers.empty();
}

#endif
