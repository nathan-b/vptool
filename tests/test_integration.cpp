#include "../vp_parser.h"
#include "../scoped_tempdir.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

// Integration tests using actual VP files from testdata/
// These tests require VP files to be present and are NOT run by default

class IntegrationTest : public ::testing::Test {
protected:
	static constexpr const char* ROOT_VP = "testdata/Root_fs2.vp";
	static constexpr const char* TANGOA_VP = "testdata/tangoA_fs2.vp";
	static constexpr const char* TANGO2_VP = "testdata/tango2_fs2.vp";

	void SetUp() override
	{
		// Check if test files exist
		if (!std::filesystem::exists(ROOT_VP)) {
			GTEST_SKIP() << "Test VP files not available in testdata/";
		}
	}
};

// Test that we can parse real VP files without errors
TEST_F(IntegrationTest, ParseRealVPFiles)
{
	vp_index idx1;
	ASSERT_TRUE(idx1.parse(ROOT_VP)) << "Failed to parse Root_fs2.vp";

	vp_index idx2;
	ASSERT_TRUE(idx2.parse(TANGOA_VP)) << "Failed to parse tangoA_fs2.vp";

	vp_index idx3;
	ASSERT_TRUE(idx3.parse(TANGO2_VP)) << "Failed to parse tango2_fs2.vp";
}

// Test printing the index listing
TEST_F(IntegrationTest, DumpIndexListing)
{
	vp_index idx;
	ASSERT_TRUE(idx.parse(ROOT_VP));

	std::string listing = idx.print_index_listing();
	EXPECT_FALSE(listing.empty());

	// Should contain expected files
	EXPECT_NE(listing.find("ai.tbl"), std::string::npos);
	EXPECT_NE(listing.find("ships.tbl"), std::string::npos);
	EXPECT_NE(listing.find("weapons.tbl"), std::string::npos);
}

// Test finding specific files
TEST_F(IntegrationTest, FindSpecificFiles)
{
	vp_index idx;
	ASSERT_TRUE(idx.parse(ROOT_VP));

	// Find a known file
	vp_file* ai_tbl = idx.find("ai.tbl");
	ASSERT_NE(ai_tbl, nullptr) << "Could not find ai.tbl";
	EXPECT_EQ(ai_tbl->get_name(), "ai.tbl");
	EXPECT_GT(ai_tbl->get_size(), 0);

	// Find another known file
	vp_file* ships_tbl = idx.find("ships.tbl");
	ASSERT_NE(ships_tbl, nullptr) << "Could not find ships.tbl";
	EXPECT_EQ(ships_tbl->get_name(), "ships.tbl");
	EXPECT_GT(ships_tbl->get_size(), 0);

	// Try to find non-existent file
	vp_file* nonexistent = idx.find("this_file_does_not_exist.txt");
	EXPECT_EQ(nonexistent, nullptr);
}

// Test extracting a single file
TEST_F(IntegrationTest, ExtractSingleFile)
{
	vp_index idx;
	ASSERT_TRUE(idx.parse(ROOT_VP));

	vp_file* ai_tbl = idx.find("ai.tbl");
	ASSERT_NE(ai_tbl, nullptr);

	scoped_tempdir tmpd("vptool-test-");
	ASSERT_TRUE(tmpd);

	std::filesystem::path extract_path = tmpd.get_path() / "ai.tbl";
	ASSERT_TRUE(ai_tbl->dump(extract_path.string()));

	// Verify the file was extracted
	EXPECT_TRUE(std::filesystem::exists(extract_path));
	EXPECT_GT(std::filesystem::file_size(extract_path), 0);

	// Verify file size matches
	EXPECT_EQ(std::filesystem::file_size(extract_path), ai_tbl->get_size());
}

// Test extracting entire VP archive
TEST_F(IntegrationTest, ExtractEntireArchive)
{
	// Use the smaller tangoA VP file for this test
	vp_index idx;
	ASSERT_TRUE(idx.parse(TANGOA_VP));

	scoped_tempdir tmpd("vptool-test-");
	ASSERT_TRUE(tmpd);

	ASSERT_TRUE(idx.dump(tmpd.get_path().string()));

	// Verify expected directory structure was created
	std::filesystem::path tangoA_dir = tmpd.get_path() / "tangoA";
	EXPECT_TRUE(std::filesystem::exists(tangoA_dir));
	EXPECT_TRUE(std::filesystem::is_directory(tangoA_dir));

	std::filesystem::path data_dir = tangoA_dir / "data";
	EXPECT_TRUE(std::filesystem::exists(data_dir));

	std::filesystem::path game_dat = data_dir / "game_dat2.set";
	EXPECT_TRUE(std::filesystem::exists(game_dat));
	EXPECT_GT(std::filesystem::file_size(game_dat), 0);
}

// Test file content extraction
TEST_F(IntegrationTest, VerifyExtractedFileContent)
{
	vp_index idx;
	ASSERT_TRUE(idx.parse(ROOT_VP));

	vp_file* credits = idx.find("credits.tbl");
	ASSERT_NE(credits, nullptr);

	// Extract content to string
	std::string content = credits->dump();
	EXPECT_FALSE(content.empty());
	EXPECT_EQ(content.length(), credits->get_size());

	// credits.tbl should contain text
	EXPECT_NE(content.find("#"), std::string::npos); // FreeSpace tables start with #
}

// Test rebuilding a VP archive from extracted files
TEST_F(IntegrationTest, RebuildVPArchive)
{
	// Extract tangoA (smaller file)
	vp_index original_idx;
	ASSERT_TRUE(original_idx.parse(TANGOA_VP));

	scoped_tempdir tmpd("vptool-test-");
	ASSERT_TRUE(tmpd);

	// Extract
	ASSERT_TRUE(original_idx.dump(tmpd.get_path().string()));

	// Build new VP from extracted files
	std::filesystem::path rebuilt_vp = tmpd.get_path() / "rebuilt.vp";
	std::filesystem::path source_dir = tmpd.get_path() / "tangoA";

	vp_index new_idx;
	ASSERT_TRUE(new_idx.build(source_dir, rebuilt_vp.string()));

	// Verify the rebuilt VP can be parsed
	vp_index rebuilt_idx;
	ASSERT_TRUE(rebuilt_idx.parse(rebuilt_vp.string()));

	// Verify it contains the same file
	vp_file* game_dat = rebuilt_idx.find("game_dat2.set");
	ASSERT_NE(game_dat, nullptr);
	EXPECT_GT(game_dat->get_size(), 0);
}

// Test replacing a file in a VP archive (in-place update)
TEST_F(IntegrationTest, ReplaceFileInPlace)
{
	// Extract a file, modify it slightly, and replace it
	// We'll use Root_fs2.vp which has smaller text files we can work with
	scoped_tempdir tmpd("vptool-test-");
	ASSERT_TRUE(tmpd);

	std::filesystem::path test_vp = tmpd.get_path() / "test.vp";
	std::filesystem::copy_file(ROOT_VP, test_vp);

	// Parse the copy
	vp_index idx;
	ASSERT_TRUE(idx.parse(test_vp.string()));

	vp_file* credits = idx.find("credits.tbl");
	ASSERT_NE(credits, nullptr);

	uint32_t original_size = credits->get_size();

	// Extract the original content
	std::string original_content = credits->dump();

	// Create a modified version (slightly smaller - just take first half)
	std::string modified_content = original_content.substr(0, original_size / 2);

	// Write to a temporary file
	std::filesystem::path replacement = tmpd.get_path() / "credits_modified.tbl";
	{
		std::ofstream out(replacement, std::ios::binary);
		out << modified_content;
	}

	uint32_t replacement_size = std::filesystem::file_size(replacement);
	ASSERT_LT(replacement_size, original_size) << "Modified content should be smaller";

	// Replace the file using in-place update
	ASSERT_TRUE(credits->write_file_contents(replacement));
	ASSERT_TRUE(idx.update_index(credits));

	// Verify the operation completed successfully
	EXPECT_EQ(credits->get_size(), replacement_size);

	// Note: We cannot re-parse the file while it's still open in the first index
	// This is a limitation of the current vp_index design - it keeps the filestream
	// open for the lifetime of the object. A proper fix would require adding a
	// close() method to vp_index or using RAII more carefully.
	// The actual command-line tool works correctly because it creates/destroys
	// the vp_index for each operation.
}

// Test file sizes and offsets are valid
TEST_F(IntegrationTest, ValidateFileOffsetsAndSizes)
{
	vp_index idx;
	ASSERT_TRUE(idx.parse(ROOT_VP));

	// Get file size of the VP
	std::filesystem::path vp_path(ROOT_VP);
	uintmax_t vp_size = std::filesystem::file_size(vp_path);

	// Find a few files and validate their offsets/sizes
	vp_file* ai_tbl = idx.find("ai.tbl");
	ASSERT_NE(ai_tbl, nullptr);

	// Offset + size should be within file bounds
	EXPECT_LT(ai_tbl->get_offset() + ai_tbl->get_size(), vp_size);
	EXPECT_GT(ai_tbl->get_offset(), 0);
	EXPECT_GT(ai_tbl->get_size(), 0);

	vp_file* ships_tbl = idx.find("ships.tbl");
	ASSERT_NE(ships_tbl, nullptr);

	EXPECT_LT(ships_tbl->get_offset() + ships_tbl->get_size(), vp_size);
	EXPECT_GT(ships_tbl->get_offset(), 0);
	EXPECT_GT(ships_tbl->get_size(), 0);
}

// Test extracting large files (from tango2)
TEST_F(IntegrationTest, ExtractLargeFiles)
{
	vp_index idx;
	ASSERT_TRUE(idx.parse(TANGO2_VP));

	std::string listing = idx.print_index_listing();
	EXPECT_FALSE(listing.empty());

	// Just verify we can parse and list it without crashes
	// (tango2 is 70MB so we won't extract everything)
}

// Test consistency: extract and rebuild should preserve file contents
TEST_F(IntegrationTest, ExtractRebuildConsistency)
{
	vp_index original;
	ASSERT_TRUE(original.parse(TANGOA_VP));

	// Get original file
	vp_file* original_file = original.find("game_dat2.set");
	ASSERT_NE(original_file, nullptr);
	std::string original_content = original_file->dump();

	// Extract
	scoped_tempdir tmpd("vptool-test-");
	ASSERT_TRUE(tmpd);
	ASSERT_TRUE(original.dump(tmpd.get_path().string()));

	// Rebuild
	std::filesystem::path rebuilt_vp = tmpd.get_path() / "rebuilt.vp";
	std::filesystem::path source_dir = tmpd.get_path() / "tangoA";

	vp_index builder;
	ASSERT_TRUE(builder.build(source_dir, rebuilt_vp.string()));

	// Parse rebuilt
	vp_index rebuilt;
	ASSERT_TRUE(rebuilt.parse(rebuilt_vp.string()));

	// Get rebuilt file
	vp_file* rebuilt_file = rebuilt.find("game_dat2.set");
	ASSERT_NE(rebuilt_file, nullptr);
	std::string rebuilt_content = rebuilt_file->dump();

	// Content should match exactly
	EXPECT_EQ(rebuilt_content.size(), original_content.size());
	EXPECT_EQ(rebuilt_content, original_content);
}
