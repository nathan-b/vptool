DBFLAGS=-g -O0 -DDEBUG
NDBFLAGS=-O2
CPPFLAGS=-Wall -Werror -std=c++20
OUTPUT=vptool

CPPFILES=main.cpp vp_parser.cpp operation.cpp scoped_tempdir.cpp
LIBS=

debug: $(CFILES)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

release: $(CFILES)
	g++ $(CPPFLAGS) $(NDBFLAGS) -o $(OUTPUT) $(CPPFILES) $(LIBS)

clean:
	-rm $(OUTPUT)
