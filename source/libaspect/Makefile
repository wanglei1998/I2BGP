CXXFLAGS=-I./src -g3 -Wall
CFLAGS=-I./src -g3 -Wall
LDFLAGS=-ldl
INSTALL:=install

OBJS=\
src/hook-manager.o \
src/context.o src/load-map.o src/breakpoint-manager.o \
src/user-context.o src/dwarf2-line.o src/debug-file.o \
src/dwarf2-info.o src/dwarf2-abbrev.o src/reader.o \
src/dwarf2-utils.o
all: libaspect.so test libfoo.so
src/%.o: CFLAGS+=-fpic
src/%.o: CXXFLAGS+=-fpic
foo.o: CFLAGS+=-fpic
libaspect.so: $(OBJS)
	$(CXX) -shared -o $@ $^
libfoo.so: foo.o
	$(CXX) -shared -o $@ $^
test: test.o
	$(CXX) -o $@ $^  $(LDFLAGS) -L. -laspect
install: all
	$(INSTALL) -d $(PREFIX)/lib
	$(INSTALL) -t $(PREFIX)/lib libaspect.so
	$(INSTALL) -d $(PREFIX)/include
	$(INSTALL) -t $(PREFIX)/include src/hook-manager.h
	$(INSTALL) -t $(PREFIX)/include src/load-map.h
	$(INSTALL) -t $(PREFIX)/include src/breakpoint-manager.h

clean:
	rm -f test src/*~ src/*.o *~ *.o libaspect.so libfoo.so 2>/dev/null

