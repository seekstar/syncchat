# You may want to modify them
INCLUDES	:= -Iinclude -I../include
LIBRARIES	:= -lssl -lcrypto -lboost_system -lodbc
TARGET		:= server
BIN			:= ../bin

CC				:= g++
FLAGS_DEBUG 	:= -std=c++17 -Wall -Wextra -fexceptions -g
FLAGS_RELEASE 	:= -std=c++17 -Wall -Wextra -fexceptions -O3

BIN_DEBUG	:= $(BIN)/debug
BIN_RELEASE := $(BIN)/release
SRC		:= src
LIB		:= lib
OBJ_DEBUG	:= obj/debug
OBJ_RELEASE	:= obj/release
DEP			:= dep

run_debug: debug
	./$(BIN_DEBUG)/$(EXECUTABLE)

run_release: release
	./$(BIN_RELEASE)/$(EXECUTABLE)

# The following does not normally need to be modified
ifeq ($(OS),Windows_NT)
EXE_EXT	:= .exe
RM 			:= del
MKDIR			:= mkdir
else
EXE_EXT	:=
RM 			:= rm -f
MKDIR			:= mkdir -p
endif

EXECUTABLE	:= $(TARGET)$(EXE_EXT)
debug: $(BIN_DEBUG)/$(EXECUTABLE)
release: $(BIN_RELEASE)/$(EXECUTABLE)
all: release

SRCS		:= $(wildcard $(SRC)/*.cpp)
OBJS_DEBUG	:= $(patsubst $(SRC)/%.cpp,$(OBJ_DEBUG)/%.o,$(SRCS))
OBJS_RELEASE:= $(patsubst $(SRC)/%.cpp,$(OBJ_RELEASE)/%.o,$(SRCS))
DEPS		:= $(patsubst $(SRC)/%.cpp,$(DEP)/%.d,$(SRCS))

clean_debug:
	$(RM) $(BIN_DEBUG)/$(EXECUTABLE) $(OBJ_DEBUG)/*.o

clean_release:
	$(RM) $(BIN_RELEASE)/$(EXECUTABLE) $(OBJ_RELEASE)/*.o

clean:	clean_debug clean_release
	$(RM) $(DEP)/*.d $(DEP)/*.d.*


$(DEP)/%.d: $(SRC)/%.cpp
	@set -e; \
	$(MKDIR) $(DEP); \
	$(RM) $@; \
	$(CC) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,^.*:,$@ $(OBJ_DEBUG)/$(patsubst $(SRC)/%.cpp,%,$<).o $(OBJ_RELEASE)/&,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

-include $(DEPS)

$(OBJ_DEBUG)/%.o: $(SRC)/%.cpp $(DEP)/%.d
	$(MKDIR) $(OBJ_DEBUG)
	$(CC) $(FLAGS_DEBUG) -c $(INCLUDES) -L$(LIB) $< -o $@

$(OBJ_RELEASE)/%.o: $(SRC)/%.cpp $(DEP)/%.d
	$(MKDIR) $(OBJ_RELEASE)
	$(CC) $(FLAGS_RELEASE) -c $(INCLUDES) -L$(LIB) $< -o $@


$(BIN_DEBUG)/%: $(OBJS_DEBUG)
	$(MKDIR) $(BIN_DEBUG)
	$(CC) $^ -o $@ $(LIBRARIES)

$(BIN_RELEASE)/%: $(OBJS_RELEASE)
	$(MKDIR) $(BIN_RELEASE)
	$(CC) $^ -o $@ $(LIBRARIES)

.PHONY: clean_debug clean_release clean debug run_debug release run_release all
