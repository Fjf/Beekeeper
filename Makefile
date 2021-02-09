

all: main.c board.c moves.c pn_tree.c list.c
	gcc main.c board.c moves.c pn_tree.c list.c -o hive
