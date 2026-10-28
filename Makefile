# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: samusanc <samusanc@student.42madrid.com>   +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/08/05 01:22:18 by samusanc          #+#    #+#              #
#    Updated: 2025/05/14 18:17:25 by samusanc         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		= ft_nm
COMPOSE := $(shell command -v docker-compose || echo "docker compose")

#=============================== INCLUDES ===============================#

INC			= -I./includes/ 
INC		   += -I./libft/ 

#============================== LIBRARIES ===============================#

SUBMODLIB	= ./libft/libft.a

#================================= GCC ==================================#

# GCC WITH LIBS AND INCLUDES
# -Wall -Wextra -Werror were commented out in the intra submission, so
# the grading/default build never saw a single compiler warning even
# though none of the crashes above were warning-detectable anyway
# (they're all logic bugs -- unchecked syscall returns, missing bounds
# checks -- not the kind of thing -Wall catches). Turned on by default
# so future changes to this project don't regress silently; the whole
# codebase already compiles clean under this. -fsanitize=address is
# left off by default on purpose: it's a debugging aid for finding
# bugs like the ones documented in README.md, not something you want
# slowing down the binary that actually gets submitted/graded.
CFLAGS		=# -fsanitize=address
CFLAGS	   += -Wall -Wextra -Werror
CC			= gcc $(CFLAGS) $(INC)

#================================= SCRS =================================#

SRCS		= \
				./src/main.c \
				./src/x64/x64_utils.c \
				./src/x86/x86_utlis.c \
				./src/utils/flags.c \
				./src/utils/print_utils.c \
				./src/utils/utils.c \

#================================= OBJS =================================#

O_DIR		= ./objects/
OBJS		= $(addprefix $(O_DIR)/, $(SRCS:.c=.o))

#========================================================================#

$(O_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(INC) -g -c $< -o $(O_DIR)/$(<:.c=.o)

# Every target below is a real file or a phony rule. This Makefile used to
# track "already built" state with empty sentinel files (.mandatory,
# .make_submodules, .clean) that were committed to git: on a fresh clone the
# link step was skipped, so `make` printed "mandatory done..." and produced no
# ft_nm at all. Sentinels are gone; make's own timestamps decide what to do.

all: $(NAME)

$(NAME): $(OBJS) $(SUBMODLIB)
	$(CC) -o $(NAME) $(OBJS) $(SUBMODLIB)
	@echo "mandatory done..."

# a header change must rebuild every object
$(OBJS): includes/ft_nm.h

#============================= SUBMODULES =============================#

LIBFT_SRCS	= $(wildcard libft/*.c libft/*/*.c libft/*.h libft/*/*.h)

$(SUBMODLIB): $(LIBFT_SRCS)
	@make -sC ./libft/ all

submodules: $(SUBMODLIB)

#=========================== CLEAN / RE ==============================#

re: fclean all

fclean: clean
	@echo "cleaning binaries..."
	@rm -f $(NAME)
	@make -sC ./libft/ fclean

submodule_fclean:
	@make -sC ./libft/ fclean

clean:
	@echo "cleaning objects..."
	@make -sC ./libft/ clean
	@rm -rf $(O_DIR)
	@echo "objects removed!"

#============================== TESTS ================================#

# Full suite (build, byte-exact diff against the system nm, edge cases,
# corrupted-ELF fuzzing). See tests/run_tests.sh and the README quick start.
test: all
	@bash tests/run_tests.sh

# Same suite inside the pinned Docker image (no local toolchain needed).
docker-test:
	$(COMPOSE) run --rm nm

#============================  DOCKER  ===============================#

docker: build up

build:
	$(COMPOSE) build

up:
	$(COMPOSE) up

down:
	$(COMPOSE) down -t 1

stop:
	if [ -n "$$(docker ps -aq)" ]; then \
		docker stop $$(docker ps -aq); \
	fi

delvol:
	if [ -n "$$(docker volume ls -qf dangling=true)" ]; then \
		docker volume rm $$(docker volume ls -qf dangling=true); \
	fi

clean-compose: 
	$(COMPOSE) down --rmi all --volumes

docker-re: clean-compose docker

docker-fclean: down docker-clean delvol
	docker system prune -a -f --volumes
	rm -rf cli/src/badapple/frames/
	rm -rf .apple

docker-clean: stop
	if [ -n "$$(docker ps -aq)" ]; then \
		docker rm $$(docker ps -aq); \
	fi


.PHONY: all clean fclean re test docker-test submodules submodule_fclean docker docker-clean docker-fclean docker-re stop down up build delvol
