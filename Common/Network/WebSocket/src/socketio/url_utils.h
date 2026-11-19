// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once
#include <string>
#include <utility>

namespace NSNetwork
{
    namespace NSWebSocket
    {
        // Split a URL into its base (scheme + host + port) and path components.
        //
        // "http://host:8080/doc/"  → {"http://host:8080", "/doc/"}
        // "https://host/doc/"     → {"https://host",      "/doc/"}
        // "http://host"           → {"http://host",        "/socket.io/"}
        // "badurl"                → {"badurl",             "/socket.io/"}
        //
        // The path is used as the Engine.IO endpoint (client_options::engineio_path).
        // When the incoming URL contains no path the socket.io default is returned.
        inline std::pair<std::string, std::string> split_url(const std::string& url)
        {
            const auto scheme_end = url.find("://");
            if (scheme_end == std::string::npos)
                return {url, "/socket.io/"};
            const auto path_start = url.find('/', scheme_end + 3);
            if (path_start == std::string::npos)
                return {url, "/socket.io/"};
            return {url.substr(0, path_start), url.substr(path_start)};
        }
    }
}
