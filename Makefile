INCLUDES	:= -Iinclude
BIN			:= bin

CC				:= g++
FLAGS		 	:= -std=c++17 -Wall -Wextra -fexceptions -O3

SRC		:= src
OBJ		:= obj
DEP			:= dep

SRCS		:= $(wildcard $(SRC)/*.cpp)
OBJS	:= $(patsubst $(SRC)/%.cpp,$(OBJ)/%.o,$(SRCS))
DEPS		:= $(patsubst $(SRC)/%.cpp,$(DEP)/%.d,$(SRCS))

ifeq ($(OS),Windows_NT)
RM 			:= del
MKDIR			:= mkdir
else
RM 			:= rm -f
MKDIR			:= mkdir -p
endif

$(DEP)/%.d: $(SRC)/%.cpp
	@set -e; \
	$(MKDIR) $(DEP); \
	$(RM) $@; \
	$(CC) -MM $(INCLUDES) $< > $@.$$$$; \
	sed 's,^.*:,$@ $(OBJ)/&,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

-include $(DEPS)

$(OBJ)/%.o: $(SRC)/%.cpp $(DEP)/%.d
	$(MKDIR) $(OBJ)
	$(CC) $(FLAGS) -c $(INCLUDES) $< -o $@


clean:
	$(RM) $(DEP)/*.d $(DEP)/*.d.* $(OBJ)/*.o
	cd server && $(MAKE) clean


debug: $(OBJS)
	cd server && $(MAKE) debug

release: $(OBJS)
	cd server && $(MAKE) release

server: release
all: release

.PHONY: clean debug release server all
