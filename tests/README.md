# vptool Test Suite

This directory contains test suites for vptool using Google Test.

## Test Organization

### Unit Tests (Default)

Unit tests verify individual components and don't require external VP files:

- **test_operation.cpp**: Command-line argument parsing
- **test_scoped_tempdir.cpp**: Temporary directory management
- **test_vp_parser.cpp**: VP file parsing with synthetic test files

**Run unit tests:**
```bash
make test
```

### Integration Tests (Requires VP Files)

Integration tests use real VP files from the `testdata/` directory to test actual program functionality:

- **test_integration.cpp**: End-to-end testing with real FreeSpace 2 VP files
  - Parsing real VP archives
  - Extracting files and directories
  - Rebuilding VP archives
  - File replacement operations
  - Content verification

**Run integration tests:**
```bash
make integration-test
```

**Note:** Integration tests require VP files in `testdata/`. These files are not checked into the repository due to copyright restrictions.

### Run All Tests

```bash
make all-tests
```

## Test Coverage Summary

### Unit Tests (19 tests)
- ✅ Operation parsing (short/long form, validation, error handling)
- ✅ Temporary directory creation and cleanup
- ✅ VP file format validation
- ✅ Security fixes (bounds checking, file size validation)

### Integration Tests (11 tests)
- ✅ Parse real VP files (Root_fs2.vp, tango2_fs2.vp, tangoA_fs2.vp)
- ✅ Dump directory index
- ✅ Find specific files
- ✅ Extract single files
- ✅ Extract entire archives
- ✅ Verify file contents
- ✅ Rebuild VP archives from extracted files
- ✅ In-place file replacement
- ✅ File offset and size validation
- ✅ Large file handling
- ✅ Extract-rebuild consistency verification

## Expected Test Data

Integration tests expect the following VP files in `testdata/`:
- `Root_fs2.vp` - FreeSpace 2 root package (~5.4MB)
- `tangoA_fs2.vp` - FreeSpace 2 campaign data (~25MB)
- `tango2_fs2.vp` - FreeSpace 2 campaign data (~70MB)

If these files are not present, integration tests will be skipped automatically.

## Adding New Tests

### Unit Tests
Add new unit tests to the appropriate test file or create a new one following the naming convention `test_<component>.cpp`. Update the `TEST_SOURCES` variable in the Makefile.

### Integration Tests
Add new integration tests to `test_integration.cpp` using the `IntegrationTest` fixture. Make sure they gracefully skip if VP files are not available.

## Test Binary Outputs

- `vptool_tests` - Unit test binary
- `vptool_integration_tests` - Integration test binary

Both binaries are excluded from version control via `.gitignore`.
