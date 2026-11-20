NAME = codexion
CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread
CPPFLAGS = -I.
SRC = main.c basic_code/ft_atoi.c \
	src/a1/error_check.c src/a1/input.c \
	src/a2/coder.c src/a2/shared.c src/a2/simulation.c \
	src/a3/start_coders.c src/heap/heap.c src/heap/pop.c \
	src/runtime/time.c src/runtime/log.c src/runtime/dongles.c \
	src/runtime/scheduler.c src/runtime/coder_routine.c src/runtime/monitor.c
OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.c codexion.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ) tests/test_scheduler

fclean: clean
	$(RM) $(NAME)

re: fclean
	$(MAKE) all

test: $(NAME)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_scheduler.c \
		$(filter-out main.o,$(OBJ)) -o tests/test_scheduler
	./tests/test_scheduler
	python3 -B tests/test_codexion.py

.PHONY: all clean fclean re test
