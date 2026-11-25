// SPDX-FileCopyrightText: 2010-2023 Ascensio System SIA
// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "socketio_internal_private.h"

namespace NSNetwork
{
    namespace NSWebSocket
    {
        CIOWebSocket_private_tls::CIOWebSocket_private_tls(CIOWebSocket* base)
            : CIOWebSocket_private(base)
        {
            m_base = base;
        }

        CIOWebSocket_private_tls::~CIOWebSocket_private_tls()
        {
            close();
        }

        void CIOWebSocket_private_tls::open(const std::map<std::string, std::string>& query)
        {
            auto [base_url, engine_path] = split_url(m_base->url);

            sioxx::client_options opts;
            opts.engineio_path       = engine_path;
            opts.reconnect_attempts  = m_reconnect_attempts;
            opts.reconnect_delay     = m_reconnect_delay;
            opts.reconnect_delay_max = m_reconnect_delay_max;

            // Remaining query params (after extracting the auth token) go
            // into the Engine.IO handshake query string.
            opts.query = query;
            sioxx::json auth;
            auto it = opts.query.find("token");
            if (it != opts.query.end())
            {
                auth = {{"token", it->second}};
                opts.query.erase(it);
            }

            m_socket = std::make_unique<sioxx::client>(std::move(opts));

            m_socket->set_open_listener([this]() {
                CTemporaryCS oCS(&m_oCS_Events);
                m_connecting_in_process = false;
                if (!m_closing_in_progress)
                { m_base->listener->onOpen(); }
            });

            m_socket->set_close_listener([this](const std::string&) {
                CTemporaryCS oCS(&m_oCS_Events);
                m_connecting_in_process = false;
                if (!m_closing_in_progress)
                { m_base->listener->onClose(0, ""); }
            });

            m_socket->set_fail_listener([this]() {
                CTemporaryCS oCS(&m_oCS_Events);
                m_connecting_in_process = false;
                if (!m_closing_in_progress)
                { m_base->listener->onError(""); }
            });

            // DNS, TCP and TLS failures arrive here; set_fail_listener only
            // fires once reconnect attempts are exhausted.
            m_socket->set_error_listener([this](const std::string& reason) {
                CTemporaryCS oCS(&m_oCS_Events);
                m_connecting_in_process = false;
                if (!m_closing_in_progress)
                { m_base->listener->onError(reason); }
            });

            m_socket->set_reconnecting_listener([this]() {
                CTemporaryCS oCS(&m_oCS_Events);
                m_connecting_in_process = true;
            });

            m_socket->set_reconnect_listener([this](unsigned, unsigned) {
                CTemporaryCS oCS(&m_oCS_Events);
                m_connecting_in_process = false;
            });

            m_ns_socket = m_socket->socket("/", auth);
            m_ns_socket->on("message", [this](const std::string&, sioxx::message data) {
                CTemporaryCS oCS(&m_oCS_Events);
                if (m_closing_in_progress)
                { return; }
                // sioxx packs all emit() arguments into a JSON array; unwrap the first one.
                const sioxx::message& payload = (data.is_array() && !data.empty()) ? data[0] : data;
                if (payload.is_null())
                { m_base->listener->onMessage("null"); }
                else if (payload.is_string())
                { m_base->listener->onMessage(payload.get<std::string>()); }
                else if (payload.is_object())
                { m_base->listener->onMessage(payload.dump()); }
                else
                { m_base->listener->onMessage(""); }
            });

            m_connecting_in_process = true;
            m_socket->connect(base_url);
        }

        void CIOWebSocket_private_tls::send(const std::string& message_str)
        {
            if (!m_ns_socket)
            { return; }
            if (!message_str.empty() &&
                (message_str.front() == '{' || message_str.front() == '['))
            {
                try
                {
                    sioxx::json parsed = sioxx::json::parse(message_str);
                    // sioxx unpacks a top-level array as separate emit() arguments;
                    // wrap it so the receiver always gets a single payload.
                    if (parsed.is_array())
                    { m_ns_socket->emit("message", sioxx::json::array({std::move(parsed)})); }
                    else
                    { m_ns_socket->emit("message", std::move(parsed)); }
                    return;
                }
                catch (const sioxx::json::exception&) {}
            }
            m_ns_socket->emit("message", sioxx::json(message_str));
        }

        void CIOWebSocket_private_tls::close()
        {
            CTemporaryCS oCS(&m_oCS);

            m_oCS_Events.Enter();
            m_closing_in_progress = true;
            m_oCS_Events.Leave();

            if (m_socket)
            {
                m_ns_socket.reset();
                m_socket->sync_close();
                m_socket.reset();
            }
        }

        void CIOWebSocket_private_tls::setReconnectInfo(const int& attemtCount, const int& delay, const int& delayMax)
        {
            // sioxx copies options into the client at connect() time, so these
            // values only take effect on the next open() call.
            m_reconnect_attempts  = attemtCount;
            m_reconnect_delay     = std::chrono::milliseconds(delay);
            m_reconnect_delay_max = std::chrono::milliseconds(delayMax);
        }
    }
}
