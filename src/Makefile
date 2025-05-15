CC = gcc
CXX = g++
CFLAGS += -Wall -pipe -O3
#CFLAGS += -Wall -pipe -g -fsanitize=address	# for debug
CXXFLAGS := $(CFLAGS)
LOBJS = ssw.o
LCPPOBJS = ssw_cpp.o
PROG = ssw_v1.2.5M
LIB = libssw.so
EXAMPLE = example_c
EXAMPLE_CPP = example_cpp
JAVA_JAR = ssw.jar
JAVA_LIB = libsswjni.so
JAVA_INLCUDES = -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
JAVA_OBJ = ssw/Aligner.class ssw/Alignment.class ssw/Example.class

.PHONY: all default java clean universal

default: $(PROG) $(EXAMPLE) $(EXAMPLE_CPP) $(LIB)
core: $(PROG)
java: java_home $(JAVA_JAR) $(JAVA_LIB)
all: default java
universal: ssw_mac_universal

# Target for creating a fat (universal) binary on macOS
ssw_mac_universal: ssw_arm ssw_intel
	@if [ "$(shell uname)" = "Darwin" ]; then \
		echo "Creating fat binary for macOS..."; \
		lipo -create -output $(PROG)_universal ssw_arm ssw_intel; \
		mv $(PROG)_universal $(PROG); \
		rm -f ssw_arm ssw_intel; \
		echo "Fat binary created as $(PROG)"; \
	else \
		echo "The 'fat' target is only supported on macOS"; \
		exit 1; \
	fi

# Compile for ARM architecture
ssw_arm: main.c kseq.h ssw.c ssw.h
	$(CC) -o $@ main.c ssw.c $(CFLAGS) $(LDFLAGS) -lm -lz -arch arm64 -march=armv8-a+fp+simd+crypto+crc -mmacosx-version-min=11.0

# Compile for Intel architecture
ssw_intel: main.c kseq.h ssw.c ssw.h
	$(CC) -o $@ main.c ssw.c $(CFLAGS) $(LDFLAGS) -lm -lz -arch x86_64 -mmacosx-version-min=10.14

$(LIB): ssw.c ssw.h
	$(CC) $(CFLAGS) -fPIC -shared -rdynamic -o $@ $<

$(PROG): main.c kseq.h

$(EXAMPLE): example.c

ifdef __arm__ # (M1)
$(PROG) $(EXAMPLE): $(LOBJS)
	$(CC) -o $@ $(filter-out %.h,$^) $(CFLAGS) $(LDFLAGS) -lm -lz -march=armv8-a+fp+simd+crypto+crc
else # x86(Intel)
$(PROG) $(EXAMPLE): $(LOBJS)
	$(CC) -o $@ $(filter-out %.h,$^) $(CFLAGS) $(LDFLAGS) -lm -lz
endif

$(EXAMPLE_CPP): example.cpp $(LOBJS) $(LCPPOBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) -lm -lz

$(JAVA_LIB): sswjni.c ssw.c ssw.h
	$(CC) $(CFLAGS) $(JAVA_INLCUDES) -fPIC -shared -rdynamic -o $@ $< ssw.c

$(JAVA_JAR): $(JAVA_OBJ)
	jar cvfe $@ ssw.Example $^

%.class: %.java
	javac -cp ./ $<

java_home:
ifndef JAVA_HOME
	@$(ECHO) "ERROR: Your JAVA_HOME environment variable is not set."
	exit 1
endif

ssw.o: ssw.c ssw.h
	$(CC) -c -o $@ $< $(CFLAGS)

ssw_cpp.o: ssw_cpp.cpp ssw_cpp.h ssw.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	-rm -f $(LOBJS) $(LCPPOBJS) $(PROG) $(LIB) $(EXAMPLE) $(EXAMPLE_CPP) $(JAVA_LIB) $(JAVA_JAR) $(JAVA_OBJ) ssw_arm ssw_intel *~
