NAME      = webserv
CXX       = c++
CXXFLAGS  = -Wall -Wextra -Werror -std=c++98 -I$(INC_DIR)

RM        = rm -rf
MKDIR     = mkdir -p

SRC_DIR   = src
INC_DIR   = inc
BUILD_DIR = build
BIN_DIR   = bin

SRCS      = $(shell find $(SRC_DIR) -name '*.cpp')
OBJS      = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

all: $(BIN_DIR)/$(NAME)

$(BIN_DIR)/$(NAME): $(OBJS)
	$(MKDIR) $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(BIN_DIR)/$(NAME)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(MKDIR) $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(BUILD_DIR)

fclean: clean
	$(RM) $(BIN_DIR)

re: fclean all

debug: CXXFLAGS += -DDEBUG_MODE=1
debug: re

.PHONY: all clean fclean re debug
