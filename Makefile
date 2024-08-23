NAME = ft_traceroute

#########
RM = rm -rf
CC = cc
CFLAGS = -Werror -Wextra -Wall
LDFLAGS = -lm
RELEASE_CFLAGS = $(CFLAGS) -DNDEBUG
#########

#########
FILES = main traceroute

SRC = $(addsuffix .c, $(FILES))

vpath %.c srcs srcs/traceroute
#########

#########
OBJ_DIR = objs
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))
DEP = $(addsuffix .d, $(basename $(OBJ)))
#########

#########
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	${CC} -MMD $(CFLAGS) -c -Iinc -Isrcs/traceroute $< -o $@

all: 
	$(MAKE) $(NAME)

$(NAME): $(OBJ) Makefile
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME) $(LDFLAGS)
	@echo "EVERYTHING DONE  "
	@./.add_path.sh

release: CFLAGS = $(RELEASE_CFLAGS)
release: re
	@echo "RELEASE BUILD DONE  "

clean:
	$(RM) $(OBJ) $(DEP)
	$(RM) -r $(OBJ_DIR)
	@echo "OBJECTS REMOVED   "

fclean: clean
	$(RM) $(NAME)
	@echo "EVERYTHING REMOVED   "

re:	fclean all

.PHONY: all clean fclean re release

-include $(DEP)
