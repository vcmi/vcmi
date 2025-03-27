/*
 * NetworkDefines.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/asio.hpp>

#include "NetworkInterface.h"

VCMI_LIB_NAMESPACE_BEGIN

#if BOOST_VERSION >= 106600
using NetworkContext = boost::asio::io_context;
#else
using NetworkContext = boost::asio::io_service;
#endif
using NetworkSocket = boost::asio::ip::tcp::socket;
using NetworkAcceptor = boost::asio::ip::tcp::acceptor;
using NetworkBuffer = boost::asio::streambuf;
using NetworkTimer = boost::asio::steady_timer;

VCMI_LIB_NAMESPACE_END
