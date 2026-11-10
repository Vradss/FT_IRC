# ANSI Colors
RED=\033[0;31m
GREEN=\033[0;32m
YELLOW=\033[0;33m
BLUE=\033[0;34m
MAGENTA=\033[0;35m
CYAN=\033[0;36m
RESET=\033[0m

# Emojis
SUCCESS_EMOJI=✨
CLEAN_EMOJI=🧹
RECYCLE_EMOJI=♻️
WARNING_EMOJI=⚠️
ERROR_EMOJI=❌
COOL_EMOJI=😎

NAME = ircserv

SRC_DIR  = srcs
INC_DIR  = includes
SRCS     = main.cpp \
           Server.cpp \
           Client.cpp

OBJ_DIR = obj
OBJS     = $(addprefix $(OBJ_DIR)/, $(SRCS:.cpp=.o))
DEPS     = $(OBJS:.o=.d)


CXX = c++
RM = rm -f
MKDIR = mkdir -p
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
CPPFLAGS = -MMD -MP -I$(INC_DIR)

all: ${NAME}

$(OBJ_DIR):
	@$(MKDIR) $(OBJ_DIR)
	@echo "${BLUE}Carpeta $(OBJ_DIR) creada${RESET}"

${NAME}: ${OBJS}
	@${CXX} ${CXXFLAGS} ${OBJS} -o ${NAME}
	@echo "${COOL_EMOJI} ${GREEN} Successful compilation! ${RESET}"


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "${CYAN} Compiling $<...${RESET}"
	@${CXX} ${CXXFLAGS} $(CPPFLAGS) -c $< -o $@

-include ${DEPS}

clean:
	@echo "${CLEAN_EMOJI} ${MAGENTA}Cleaning files...${RESET}"
	@${RM} -r ${OBJ_DIR}

fclean: clean
	@echo "${RECYCLE_EMOJI} ${YELLOW} Removing executable...${RESET}"
	@${RM} ${NAME}

re: fclean all

.PHONY: all clean fclean re

