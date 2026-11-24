/*
 * SPDX-FileCopyrightText: 2026 Euro-Office contributors
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "gtest/gtest.h"

#include "../SystemUtils.h"
#include "../Directory.h"
#include "../File.h"

#if !defined(_WIN32)
#include <stdlib.h>
#endif

TEST(SystemUtils, BuildBrandingIsNotOnlyoffice)
{
	std::wstring branding = NSSystemUtils::GetBuildBranding();
	ASSERT_FALSE(branding.empty());
	EXPECT_EQ(std::wstring::npos, branding.find(L"ONLYOFFICE"));
	EXPECT_EQ(std::wstring::npos, branding.find(L"onlyoffice"));
#if !defined(BUILD_BRANDING_NAME)
#if defined(_LINUX) && !defined(_MAC)
	EXPECT_EQ(L"euro-office", branding);
#else
	EXPECT_EQ(L"Euro-Office", branding);
#endif
#endif
}

#if !defined(_WIN32)
// GetAppDataDir() reads $HOME on Linux and macOS, so point it at a scratch home
class SystemUtilsAppData : public ::testing::Test
{
protected:
	void SetUp() override
	{
		const char* oldHome = getenv("HOME");
		m_hadHome = (oldHome != nullptr);
		if (m_hadHome)
			m_oldHome = oldHome;

		m_home = NSFile::CFileBinary::CreateTempFileWithUniqueName(NSDirectory::GetTempPath(), L"eo_home");
		NSFile::CFileBinary::Remove(m_home);
		ASSERT_TRUE(NSDirectory::CreateDirectory(m_home));
		setenv("HOME", U_TO_UTF8(m_home).c_str(), 1);
	}

	void TearDown() override
	{
		if (m_hadHome)
			setenv("HOME", m_oldHome.c_str(), 1);
		else
			unsetenv("HOME");
		NSDirectory::DeleteDirectory(m_home);
	}

	std::wstring m_home;
	std::string m_oldHome;
	bool m_hadHome = false;
};

TEST_F(SystemUtilsAppData, UsesBrandedFolderUnderLocalShare)
{
	ASSERT_TRUE(NSDirectory::CreateDirectories(m_home + L"/.local/share"));

	std::wstring dir = NSSystemUtils::GetAppDataDir();

	EXPECT_EQ(m_home + L"/.local/share/" + NSSystemUtils::GetBuildBranding(), dir);
	EXPECT_TRUE(NSDirectory::Exists(dir));
	EXPECT_FALSE(NSDirectory::Exists(m_home + L"/.local/share/ONLYOFFICE"));
	EXPECT_FALSE(NSDirectory::Exists(m_home + L"/.local/share/onlyoffice"));
}

TEST_F(SystemUtilsAppData, FallsBackToHomeWithoutLocal)
{
	std::wstring dir = NSSystemUtils::GetAppDataDir();

	EXPECT_EQ(m_home + L"/" + NSSystemUtils::GetBuildBranding(), dir);
	EXPECT_TRUE(NSDirectory::Exists(dir));
	EXPECT_FALSE(NSDirectory::Exists(m_home + L"/ONLYOFFICE"));
}
#endif
