##
## EPITECH PROJECT, 2025
## rush3
## File description:
## Makefile
##

SRC =	$(wildcard src/*.cpp)

DEPENDS =	$(wildcard	include/)

OBJ =	$(SRC:.cpp=.o)

NAME =	myftp

CFLAGS = -std=c++20 -Wall -Wextra -Werror -g
##LDFLAGS +=	 -lncurses -lsfml-graphics -lsfml-window -lsfml-system

all:	$(NAME)

$(NAME):	$(OBJ)
	g++ -g  -o $(NAME) $(OBJ) -include $(DEPENDS) $(LDFLAGS)

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)
	rm -rf a.out

re:	fclean all

.PHONY: all $(NAME) clean fclean re
