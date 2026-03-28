/*
 * ForumLogin.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <optional>
#include <string>
#include <boost/asio/awaitable.hpp>

/// Verifies user credentials against the VCMI forum (forum.vcmi.eu, Discourse).
/// Authentication is performed programmatically via the Discourse session API;
/// no browser redirection is involved.
class ForumLogin
{
public:
	struct Result
	{
		std::string username;      ///< Canonical forum username
		std::string sessionCookie; ///< Raw Cookie header value to be stored and reused
	};

	/// Contacts the forum and attempts to authenticate with the supplied credentials.
	/// Returns Result on success (username + session cookie), or an empty optional on failure.
	static boost::asio::awaitable<std::optional<Result>> verifyCredentialsAsync(std::string username, std::string password, std::string forumHost);

	/// Checks whether a previously stored Discourse session cookie is still valid.
	/// Makes a single HTTPS request to /session/current.json.
	static boost::asio::awaitable<bool> isSessionValidAsync(std::string sessionCookie, std::string forumHost);
};
