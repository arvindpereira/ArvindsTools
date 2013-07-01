# Author: Arvind Pereira
# Makefile for Arvind's Tools Library.
# Modify the includes, sources, headers, executable and
# other flags accordingly!
INCLUDES = -I ./ -I/usr/local/include -I/usr/include
LIBS = -L/usr/lib/ -L/usr/local/lib -lm -pthread -lpthread
BOOST_MPI_LIBS = -L/usr/local/lib -lboost_mpi -lboost_serialization
BOOST_THREAD_LIBS = -L/usr/local/lib -lboost_thread -lboost_system -lboost_exception
JSON_SPIRIT_LIBS = -L/usr/local/lib -ljson_spirit

ARVINDS_TOOLS_LIBS = -L. -l_arvinds_tools

CXX_MPI = mpic++
CC = g++
DEBUG = -g
LFLAGS = -Wall $(DEBUG)
CFLAGS = -Wall -c 
PTHREAD_FLAGS = -D_POSIX_PTHREAD_SEMANTICS
SHARED_FLAGS = -fPIC -shared

TOOLS_LOC = 
ARVINDS_TOOLS_SRCS = $(TOOLS_LOC)SignalTools.cpp $(TOOLS_LOC)Client.cpp $(TOOLS_LOC)Server.cpp \
		$(TOOLS_LOC)CommandCallbackHandler.cpp $(TOOLS_LOC)TimeTools.cpp
ARVINDS_TOOLS_HDRS = $(TOOLS_LOC)SignalTools.h $(TOOLS_LOC)Server.h \
		$(TOOLS_LOC)CommandCallbackHandler.h $(TOOLS_LOC)TimeTools.h
ARVINDS_TOOLS_OBJS = $(ARVINDS_TOOLS_SRCS:.cpp=.o)

ARVINDS_TOOLS_LIB = lib_arvinds_tools.so

TOPLEVEL_FOLDER = ArvindsTools

all: $(ARVINDS_TOOLS_LIB)
	@echo "-------------------------Done------------------------------"
	@echo "Type: 'make docs' for documentation"
	@echo "Type: 'make tar'  for a code snapshot to be saved to tar.gz"

$(ARVINDS_TOOLS_LIB): $(ARVINDS_TOOLS_OBJS) $(ARVINDS_TOOLS_HDRS)
	$(CC) $(SHARED_FLAGS) -o $(ARVINDS_TOOLS_LIB) $(ARVINDS_TOOLS_OBJS)

.cpp.o: $(INCLUDES)
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

tar:
	make clean; cd ..; tar -czf $(TAR_FILE_NAME) $(TOPLEVEL_FOLDER)
	cd ..; mv *$(TOPLEVEL_FOLDER).tar.gz $(TOPLEVEL_FOLDER)/

clean:
	rm -f $(OBJ)
	rm -f $(EXE)
	rm -rf docs

install:
	cp $(ARVINDS_TOOLS_LIB) $(LIB_INSTALL_DESTINATION)
	cp *.h $(INC_INSTALL_DESTINATION)

LIB_INSTALL_DESTINATION = /usr/local/lib/
INC_INSTALL_DESTINATION = /usr/local/include/
	
TAR_FILE_NAME = `date +'%Y%m%d_%H%M%S_'`$(TOPLEVEL_FOLDER).tar.gz

OBJ = $(ARVINDS_TOOLS_OBJS)
EXE = $(ARVINDS_TOOLS_LIB)

SOURCES = $(ARVINDS_TOOLS_SRCS)
HEADERS = $(ARVINDS_TOOLS_HDRS)

docs: $(SOURCES) $(HEADERS)
	if command -v doxygen; then doxygen Doxyfile; \
	else echo "Doxygen not found. Not making documentation."; fi

