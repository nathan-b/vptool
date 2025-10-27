DBFLAGS=-g -O0 -DDEBUG
NDBFLAGS=-O2
CPPFLAGS=-Wall -Werror -std=c++20
OUTPUT=vptool
TEST_OUTPUT=vptool_tests

CPPFILES=main.cpp vp_parser.cpp operation.cpp scoped_tempdir.cpp
LIBS=

# Test files
TEST_SOURCES=tests/test_main.cpp tests/test_operation.cpp tests/test_scoped_tempdir.cpp tests/test_vp_parser.cpp
TEST_OBJECTS=vp_parser.cpp operation.cpp scoped_tempdir.cpp
TEST_LIBS=-lgtest -pthread

debug: $(CPPFILES)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

release: $(CPPFILES)
	g++ $(CPPFLAGS) $(NDBFLAGS) -o $(OUTPUT) $(CPPFILES) $(LIBS)

test: $(TEST_SOURCES) $(TEST_OBJECTS)
	g++ $(CPPFLAGS) $(DBFLAGS) -o $(TEST_OUTPUT) $(TEST_SOURCES) $(TEST_OBJECTS) $(TEST_LIBS)
	./$(TEST_OUTPUT)

clean:
	-rm $(OUTPUT) $(TEST_OUTPUT)
