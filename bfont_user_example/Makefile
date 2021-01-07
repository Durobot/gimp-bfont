SOURCES = bfont_user.c bfont_user_fonts.c
CC = gcc

# OUT_NAME specifies the name of our exectuable
OUT_NAME = bfont_user

# OS is the type of Windows, it should be defined on all Windows computers but not on Linux
ifdef OS
	# Additional include paths we'll need
	INCLUDE_PATHS = -IC:\mingw_dev_lib\include

	# Additional library paths we'll need
	LIBRARY_PATHS = -LC:\mingw_dev_lib\lib

	# Compilation options we're using
	# -Wl,-subsystem,windows gets rid of the console window
	COMPILER_FLAGS = -Wl,-subsystem,windows

	# The libraries we're linking against
	LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2
else # Linux (or Mac, maybe?)
	#INCLUDE_PATHS specifies the additional include paths we'll need
	INCLUDE_PATHS =

	#LIBRARY_PATHS specifies the additional library paths we'll need
	LIBRARY_PATHS =

	#COMPILER_FLAGS specifies the additional compilation options we're using
	COMPILER_FLAGS =

	# The libraries we're linking against
	LINKER_FLAGS = -lSDL2
endif

# This is the target that compiles our executable
all : $(SOURCES)
	$(CC) $(SOURCES) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OUT_NAME)
