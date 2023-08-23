NAME=ircserv
FILES=main.cpp Server.cpp User.cpp Channel.cpp utils.cpp
FILES_O=$(FILES:.cpp=.o)
CPPFLAGS=-Wall -Werror -Wextra -std=c++98 -fsanitize=address  -g
CXX=c++

all: $(NAME)

$(NAME): $(FILES_O)
	$(CXX)  $(CPPFLAGS) $(FILES_O) -o $(NAME)

clean:
	rm -rf $(FILES_O)

fclean: clean
	rm -rf $(NAME)

re: fclean all

bonus: all

.PHONY: all clean fclean re bonus

