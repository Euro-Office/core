/*
 * SPDX-FileCopyrightText: 2023 XWiki CryptPad Team <contact@cryptpad.org> and contributors
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Runs inside the createX2tModule() factory before the runtime starts.
 * The driver calls main1() per conversion, so main() must not run on load
 * and the runtime must stay alive between calls.
 */
Module['noInitialRun'] = true;
Module['noExitRuntime'] = true;
