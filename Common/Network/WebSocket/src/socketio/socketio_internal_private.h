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

#pragma once
#include "socketio_internal.h"
#include "url_utils.h"
#include <chrono>
#include <limits>
#include <memory>
#include <sioxx/sioxx.hpp>
#include "DesktopEditor/graphics/TemporaryCS.h"

namespace NSNetwork
{
    namespace NSWebSocket
    {
        class CIOWebSocket_private_tls : public CIOWebSocket_private
        {
        public:
            std::unique_ptr<sioxx::client> m_socket;
            std::shared_ptr<sioxx::socket> m_ns_socket;
            bool m_connecting_in_process{false};
            bool m_closing_in_progress{false};
            int  m_reconnect_attempts{std::numeric_limits<int>::max()};
            std::chrono::milliseconds m_reconnect_delay{2000};
            std::chrono::milliseconds m_reconnect_delay_max{30000};

        public:
            CIOWebSocket_private_tls(CIOWebSocket* base);
            ~CIOWebSocket_private_tls();

        public:
            virtual void open(const std::map<std::string, std::string>& query) override;
            virtual void send(const std::string& message_str) override;
            virtual void close() override;
            virtual void setReconnectInfo(const int& attemtCount, const int& delay, const int& delayMax) override;
        };
    }
}
