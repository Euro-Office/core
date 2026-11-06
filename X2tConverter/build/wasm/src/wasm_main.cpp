/*
 * SPDX-FileCopyrightText: 2023 XWiki CryptPad Team <contact@cryptpad.org> and contributors
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-or-later
 *
 * Entry point exported from x2t.wasm. The JS driver writes a params.xml into
 * the in-memory file system and calls main1 with its path, which is the same
 * as running "x2t params.xml" natively. Same shape as CryptPad's wrap-main.cpp.
 */

int main(int argc, char* argv[]);

extern "C"
{
	int main1(char* xmlPath)
	{
		char empty[] = "";
		char* argv[2] = { empty, xmlPath };
		return main(2, argv);
	}
}
