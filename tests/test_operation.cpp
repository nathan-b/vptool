#include "../operation.h"
#include <gtest/gtest.h>

// Test operation parsing with valid short form commands
TEST(OperationTest, ParseShortFormCommands)
{
	{
		const char* argv[] = { "vptool", "t", "test.vp" };
		operation op;
		ASSERT_TRUE(op.parse(3, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), DUMP_INDEX);
		EXPECT_EQ(op.get_package_filename(), "test.vp");
	}

	{
		const char* argv[] = { "vptool", "d", "test.vp", "-f", "myfile.txt" };
		operation op;
		ASSERT_TRUE(op.parse(5, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), DUMP_FILE);
		EXPECT_EQ(op.get_internal_filename(), "myfile.txt");
	}

	{
		const char* argv[] = { "vptool", "x", "test.vp", "-o", "/tmp/output" };
		operation op;
		ASSERT_TRUE(op.parse(5, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), EXTRACT_ALL);
		EXPECT_EQ(op.get_dest_path(), "/tmp/output");
	}
}

// Test operation parsing with long form commands
TEST(OperationTest, ParseLongFormCommands)
{
	{
		const char* argv[] = { "vptool", "dump-index", "test.vp" };
		operation op;
		ASSERT_TRUE(op.parse(3, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), DUMP_INDEX);
	}

	{
		const char* argv[] = { "vptool", "dump-file", "test.vp", "-f", "myfile.txt" };
		operation op;
		ASSERT_TRUE(op.parse(5, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), DUMP_FILE);
	}

	{
		const char* argv[] = { "vptool", "extract-file", "test.vp", "-f", "myfile.txt", "-o", "output.txt" };
		operation op;
		ASSERT_TRUE(op.parse(7, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), EXTRACT_FILE);
	}

	{
		const char* argv[] = { "vptool", "extract-all", "test.vp" };
		operation op;
		ASSERT_TRUE(op.parse(3, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), EXTRACT_ALL);
	}

	{
		const char* argv[] = { "vptool", "replace-file", "test.vp", "-f", "myfile.txt", "-i", "input.txt" };
		operation op;
		ASSERT_TRUE(op.parse(7, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), REPLACE_FILE);
	}

	{
		const char* argv[] = { "vptool", "build-package", "output.vp", "-i", "/path/to/data" };
		operation op;
		ASSERT_TRUE(op.parse(5, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), BUILD_PACKAGE);
	}
}

// Test that short strings don't cause crashes (Fix #1)
TEST(OperationTest, ShortStringsDoNotCrash)
{
	{
		const char* argv[] = { "vptool", "d", "test.vp" };
		operation op;
		ASSERT_TRUE(op.parse(3, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_type(), DUMP_FILE);
	}

	{
		const char* argv[] = { "vptool", "dump", "test.vp" };
		operation op;
		ASSERT_FALSE(op.parse(3, const_cast<char**>(argv)));
	}

	{
		const char* argv[] = { "vptool", "ex", "test.vp" };
		operation op;
		ASSERT_FALSE(op.parse(3, const_cast<char**>(argv)));
	}

	{
		const char* argv[] = { "vptool", "extract", "test.vp" };
		operation op;
		ASSERT_FALSE(op.parse(3, const_cast<char**>(argv)));
	}
}

// Test invalid operations
TEST(OperationTest, InvalidOperations)
{
	{
		const char* argv[] = { "vptool", "invalid", "test.vp" };
		operation op;
		EXPECT_FALSE(op.parse(3, const_cast<char**>(argv)));
	}

	{
		const char* argv[] = { "vptool", "z", "test.vp" };
		operation op;
		EXPECT_FALSE(op.parse(3, const_cast<char**>(argv)));
	}
}

// Test missing required arguments (Fix #1)
TEST(OperationTest, MissingRequiredArguments)
{
	{
		const char* argv[] = { "vptool", "x", "test.vp", "-o" };
		operation op;
		EXPECT_FALSE(op.parse(4, const_cast<char**>(argv)));
	}

	{
		const char* argv[] = { "vptool", "r", "test.vp", "-f" };
		operation op;
		EXPECT_FALSE(op.parse(4, const_cast<char**>(argv)));
	}

	{
		const char* argv[] = { "vptool", "p", "test.vp", "-i" };
		operation op;
		EXPECT_FALSE(op.parse(4, const_cast<char**>(argv)));
	}
}

// Test long form options
TEST(OperationTest, LongFormOptions)
{
	{
		const char* argv[] = { "vptool", "x", "test.vp", "--output-path", "/tmp/out" };
		operation op;
		ASSERT_TRUE(op.parse(5, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_dest_path(), "/tmp/out");
	}

	{
		const char* argv[] = { "vptool", "r", "test.vp", "--package-file", "myfile.txt", "--input-file", "input.txt" };
		operation op;
		ASSERT_TRUE(op.parse(7, const_cast<char**>(argv)));
		EXPECT_EQ(op.get_internal_filename(), "myfile.txt");
		EXPECT_EQ(op.get_src_filename(), "input.txt");
	}
}
