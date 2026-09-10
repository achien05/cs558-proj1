FLAGS = -Wall -Wextra -g -std=c++20
CC = g++

all: compile

compile: achienSalsa208.cpp
	g++ ./achienSalsa208.cpp -o achienSalsa208

clean:
	rm -f ./achienSalsa208