#include "../scoped_tempdir.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

// Test basic creation and cleanup
TEST(ScopedTempdirTest, CreatesAndCleansUpDirectory)
{
	std::filesystem::path created_path;

	{
		scoped_tempdir tmpd("test-");
		ASSERT_TRUE(tmpd);

		created_path = tmpd.get_path();
		EXPECT_TRUE(std::filesystem::exists(created_path));
		EXPECT_TRUE(std::filesystem::is_directory(created_path));

		// Verify the prefix is in the path
		std::string path_str = created_path.string();
		EXPECT_NE(path_str.find("test-"), std::string::npos);
	}

	// After scoped_tempdir goes out of scope, directory should be cleaned up
	EXPECT_FALSE(std::filesystem::exists(created_path));
}

// Test that files within the temp directory are cleaned up
TEST(ScopedTempdirTest, CleansUpFilesInDirectory)
{
	std::filesystem::path test_file;

	{
		scoped_tempdir tmpd("test-");
		ASSERT_TRUE(tmpd);

		test_file = tmpd.get_path() / "testfile.txt";
		std::ofstream outfile(test_file);
		outfile << "test content";
		outfile.close();

		EXPECT_TRUE(std::filesystem::exists(test_file));
	}

	// File should be cleaned up along with directory
	EXPECT_FALSE(std::filesystem::exists(test_file));
}

// Test operator/ for path concatenation
TEST(ScopedTempdirTest, PathConcatenationOperator)
{
	scoped_tempdir tmpd("test-");
	ASSERT_TRUE(tmpd);

	std::filesystem::path combined = tmpd / "subdir" / "file.txt";
	std::string combined_str = combined.string();

	EXPECT_NE(combined_str.find("test-"), std::string::npos);
	EXPECT_NE(combined_str.find("subdir"), std::string::npos);
	EXPECT_NE(combined_str.find("file.txt"), std::string::npos);
}

// Test conversion operators
TEST(ScopedTempdirTest, ConversionOperators)
{
	scoped_tempdir tmpd("test-");
	ASSERT_TRUE(tmpd);

	// Test conversion to std::filesystem::path
	std::filesystem::path as_path = tmpd;
	EXPECT_TRUE(std::filesystem::exists(as_path));

	// Test conversion to std::string
	std::string as_string = tmpd;
	EXPECT_FALSE(as_string.empty());
	EXPECT_NE(as_string.find("test-"), std::string::npos);

	// Test bool conversion
	EXPECT_TRUE(static_cast<bool>(tmpd));
}

// Test nested directory creation
TEST(ScopedTempdirTest, NestedDirectories)
{
	scoped_tempdir tmpd("test-");
	ASSERT_TRUE(tmpd);

	std::filesystem::path nested = tmpd.get_path() / "level1" / "level2" / "level3";
	std::filesystem::create_directories(nested);

	EXPECT_TRUE(std::filesystem::exists(nested));
	EXPECT_TRUE(std::filesystem::is_directory(nested));
}
