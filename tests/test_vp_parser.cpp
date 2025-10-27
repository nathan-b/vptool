#include "../vp_parser.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstring>

// VP file format structures (copied from vp_parser.cpp for testing)
const uint32_t vp_sig = 0x50565056;

struct vp_header {
	char header[4];
	int version;
	int diroffset;
	int direntries;
};

struct vp_direntry {
	int offset;
	int size;
	char name[32];
	int timestamp;
};

// Helper to create a minimal valid VP file for testing
class VPFileFixture : public ::testing::Test {
protected:
	std::filesystem::path test_vp_path;

	void SetUp() override
	{
		test_vp_path = std::filesystem::temp_directory_path() / "test_vp_file.vp";
	}

	void TearDown() override
	{
		if (std::filesystem::exists(test_vp_path)) {
			std::filesystem::remove(test_vp_path);
		}
	}

	// Create a minimal valid VP file
	void CreateValidVPFile()
	{
		std::ofstream vp(test_vp_path, std::ios::binary);

		// Write header
		vp_header hdr;
		memcpy(hdr.header, "VPVP", 4);
		hdr.version = 2;
		hdr.diroffset = sizeof(vp_header) + 11; // After header + file data
		hdr.direntries = 3; // dir entry + file entry + updir

		vp.write((char*)&hdr, sizeof(hdr));

		// Write a simple file data
		const char* file_content = "Hello World";
		vp.write(file_content, 11);

		// Write directory index
		// First, a directory entry for "data"
		vp_direntry dir_entry;
		memset(&dir_entry, 0, sizeof(dir_entry));
		dir_entry.offset = 0;
		dir_entry.size = 0;
		strcpy(dir_entry.name, "data");
		dir_entry.timestamp = 0;
		vp.write((char*)&dir_entry, sizeof(dir_entry));

		// Then the file entry
		vp_direntry file_entry;
		memset(&file_entry, 0, sizeof(file_entry));
		file_entry.offset = sizeof(vp_header);
		file_entry.size = 11;
		strcpy(file_entry.name, "test.txt");
		file_entry.timestamp = 0;
		vp.write((char*)&file_entry, sizeof(file_entry));

		// Updir marker to exit "data" directory
		vp_direntry updir;
		memset(&updir, 0, sizeof(updir));
		strcpy(updir.name, "..");
		vp.write((char*)&updir, sizeof(updir));

		vp.close();
	}

	// Create a VP file with invalid signature
	void CreateInvalidSignatureVPFile()
	{
		std::ofstream vp(test_vp_path, std::ios::binary);

		vp_header hdr;
		memcpy(hdr.header, "XXXX", 4);
		hdr.version = 2;
		hdr.diroffset = sizeof(vp_header);
		hdr.direntries = 0;

		vp.write((char*)&hdr, sizeof(hdr));
		vp.close();
	}

	// Create a VP file with diroffset beyond file size (Fix #15)
	void CreateInvalidDirOffsetVPFile()
	{
		std::ofstream vp(test_vp_path, std::ios::binary);

		vp_header hdr;
		memcpy(hdr.header, "VPVP", 4);
		hdr.version = 2;
		hdr.diroffset = 999999; // Way beyond actual file size
		hdr.direntries = 0;

		vp.write((char*)&hdr, sizeof(hdr));
		vp.close();
	}

	// Create a VP file with excessive number of entries (Fix #15)
	void CreateExcessiveEntriesVPFile()
	{
		std::ofstream vp(test_vp_path, std::ios::binary);

		vp_header hdr;
		memcpy(hdr.header, "VPVP", 4);
		hdr.version = 2;
		hdr.diroffset = sizeof(vp_header);
		hdr.direntries = 10000000; // Way too many entries

		vp.write((char*)&hdr, sizeof(hdr));
		vp.close();
	}

	// Create a VP file with a file entry that extends beyond package size (Fix #15)
	void CreateFileExtendsBeyondPackageVPFile()
	{
		std::ofstream vp(test_vp_path, std::ios::binary);

		vp_header hdr;
		memcpy(hdr.header, "VPVP", 4);
		hdr.version = 2;
		hdr.diroffset = sizeof(vp_header);
		hdr.direntries = 2;

		vp.write((char*)&hdr, sizeof(hdr));

		// File entry with offset and size that extend beyond file
		vp_direntry file_entry;
		memset(&file_entry, 0, sizeof(file_entry));
		file_entry.offset = sizeof(vp_header);
		file_entry.size = 999999; // Huge size
		strcpy(file_entry.name, "test.txt");
		file_entry.timestamp = 0;
		vp.write((char*)&file_entry, sizeof(file_entry));

		// Updir marker
		vp_direntry updir;
		memset(&updir, 0, sizeof(updir));
		strcpy(updir.name, "..");
		vp.write((char*)&updir, sizeof(updir));

		vp.close();
	}
};

// Test parsing a valid VP file
TEST_F(VPFileFixture, ParseValidVPFile)
{
	CreateValidVPFile();

	vp_index idx;
	ASSERT_TRUE(idx.parse(test_vp_path.string()));

	// Find the file we created
	vp_file* file = idx.find("test.txt");
	ASSERT_NE(file, nullptr);
	EXPECT_EQ(file->get_name(), "test.txt");
	EXPECT_EQ(file->get_size(), 11);
}

// Test parsing with invalid signature
TEST_F(VPFileFixture, RejectsInvalidSignature)
{
	CreateInvalidSignatureVPFile();

	vp_index idx;
	EXPECT_FALSE(idx.parse(test_vp_path.string()));
}

// Test parsing with invalid diroffset (Fix #15)
TEST_F(VPFileFixture, RejectsInvalidDirOffset)
{
	CreateInvalidDirOffsetVPFile();

	vp_index idx;
	EXPECT_FALSE(idx.parse(test_vp_path.string()));
}

// Test parsing with excessive number of entries (Fix #15)
TEST_F(VPFileFixture, RejectsExcessiveEntries)
{
	CreateExcessiveEntriesVPFile();

	vp_index idx;
	EXPECT_FALSE(idx.parse(test_vp_path.string()));
}

// Test parsing when file extends beyond package size (Fix #15)
TEST_F(VPFileFixture, RejectsFileExtendingBeyondPackage)
{
	CreateFileExtendsBeyondPackageVPFile();

	vp_index idx;
	EXPECT_FALSE(idx.parse(test_vp_path.string()));
}

// Test parsing non-existent file
TEST_F(VPFileFixture, RejectsNonExistentFile)
{
	vp_index idx;
	EXPECT_FALSE(idx.parse("/nonexistent/path/file.vp"));
}

// Test find returns nullptr for non-existent files
TEST_F(VPFileFixture, FindReturnsNullForNonExistent)
{
	CreateValidVPFile();

	vp_index idx;
	ASSERT_TRUE(idx.parse(test_vp_path.string()));

	vp_file* file = idx.find("nonexistent.txt");
	EXPECT_EQ(file, nullptr);
}

// Test extracting file content
TEST_F(VPFileFixture, ExtractFileContent)
{
	CreateValidVPFile();

	vp_index idx;
	ASSERT_TRUE(idx.parse(test_vp_path.string()));

	vp_file* file = idx.find("test.txt");
	ASSERT_NE(file, nullptr);

	std::string content = file->dump();
	EXPECT_EQ(content, "Hello World");
}
