##
## Author: Mickaël BLET
##

#-------------------------------------------------------------------------------
# common
#-------------------------------------------------------------------------------

# choose your compilation mode at 'make' call
#	modes: (debug, release, lib_debug, lib_release, test)
#	default value: debug
MODE					=	lib_debug

# define version of module
#	default value: 0.0.0
VERSION					=	1.0.0

# name of your binary
#	default value: (name of Makefile current directory)
BINARY_NAME				=	udtcp

# name of your library
#	default value: (name of Makefile current directory)
LIBRARY_NAME			=	udtcp

#-------------------------------------------------------------------------------
# directories
#-------------------------------------------------------------------------------

# destination path of your binaries (not forget the last '/')
#	default value: bin/
BINARY_DIRECTORY		=	../bin/

# destination path of your libraries (not forget the last '/')
#	default value: lib/
LIBRARY_DIRECTORY		=	../lib/

# source path (not forget the last '/')
#	default value: src/
SOURCE_DIRECTORY		=	./src/

# source test path (not forget the last '/')
#	default value: test/
TEST_DIRECTORY			=	./test/

# include path (not forget the last '/')
#	default value: include/
INCLUDE_DIRECTORY		=	./include/

# object path (not forget the last '/')
#	default value: obj/
OBJECT_DIRECTORY		=	./obj/

#-------------------------------------------------------------------------------
# compilation
#-------------------------------------------------------------------------------

# extention of source file
#	default value: .c
SOURCE_EXTENTION		=	.c

# exclude source for binary
BINARY_EXCLUDE_SOURCE	=

# exclude source for library
LIBRARY_EXCLUDE_SOURCE	=

# exclude source for test
TEST_EXCLUDE_SOURCE		=

# compilation line:
#	$(COMPILER) $(FLAGS) ... $(OPTIONS)

DEBUG_COMPILER			=	$(CC)
RELEASE_COMPILER		=	$(CC)
TEST_COMPILER			=	$(CXX)

DEBUG_FLAGS				=	-std=c99 -Wall -Wextra -ggdb3
RELEASE_FLAGS			=	-O2 -Wall -Wextra -Werror
TEST_FLAGS				=	-std=c++11 -Wall -Wextra -ggdb3

DEBUG_OPTIONS			=	-DVERSION=\"$(VERSION)\" -DDEBUG \
							-I$(INCLUDE_DIRECTORY) -lpthread
RELEASE_OPTIONS			=	-DVERSION=\"$(VERSION)\" \
							-I$(INCLUDE_DIRECTORY) -lpthread
TEST_OPTIONS			=	-DVERSION=\"$(VERSION)\" -DDEBUG \
							-I$(INCLUDE_DIRECTORY) \
							-L$(LIBRARY_DIRECTORY) \
							-l$(LIBRARY_NAME)-debug \
							-lgtest -lgtest_main -lgmock -lpthread -ldl

DEBUG_DEPENDENCIES		=
RELEASE_DEPENDENCIES	=
TEST_DEPENDENCIES		=	$(LIBRARY_DIRECTORY)lib$(LIBRARY_NAME)-debug.a

include ../module.mk

exe_test:	test
	@$(foreach bin,$(BINARIES_TEST),printf %80s |tr " " "="; echo "\n$(bin)\n";\
	$(bin) || exit;)

PHONY:
