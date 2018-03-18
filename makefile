all:
	g++ -DNDEBUG -std=c++98 -o unify main.cpp

dev:
	g++ -g -Wall -std=c++98 -o unify main.cpp
