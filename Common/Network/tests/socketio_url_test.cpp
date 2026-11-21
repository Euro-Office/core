// SPDX-FileCopyrightText: 2026 Euro-Office contributors
// SPDX-License-Identifier: AGPL-3.0-only

#include <gtest/gtest.h>
#include "../WebSocket/src/socketio/url_utils.h"

using NSNetwork::NSWebSocket::split_url;

TEST(SplitUrl, HttpWithPath)
{
    auto [base, path] = split_url("http://localhost:8080/doc/");
    EXPECT_EQ(base, "http://localhost:8080");
    EXPECT_EQ(path, "/doc/");
}

TEST(SplitUrl, HttpsWithPath)
{
    auto [base, path] = split_url("https://example.com/doc/");
    EXPECT_EQ(base, "https://example.com");
    EXPECT_EQ(path, "/doc/");
}

TEST(SplitUrl, HttpsWithPortAndPath)
{
    auto [base, path] = split_url("https://example.com:4433/custom/path/");
    EXPECT_EQ(base, "https://example.com:4433");
    EXPECT_EQ(path, "/custom/path/");
}

TEST(SplitUrl, NoPathFallsBackToSocketIoDefault)
{
    auto [base, path] = split_url("http://localhost:8080");
    EXPECT_EQ(base, "http://localhost:8080");
    EXPECT_EQ(path, "/socket.io/");
}

TEST(SplitUrl, NoSchemeReturnedAsBase)
{
    auto [base, path] = split_url("notaurl");
    EXPECT_EQ(base, "notaurl");
    EXPECT_EQ(path, "/socket.io/");
}

TEST(SplitUrl, EmptyStringIsHandledGracefully)
{
    auto [base, path] = split_url("");
    EXPECT_EQ(base, "");
    EXPECT_EQ(path, "/socket.io/");
}
