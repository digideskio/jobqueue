CFLAGS := -std=c++1y -g -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-c99-extensions -Wno-padded

default:
	clang++ $(CFLAGS) main.cpp -o jobqueue
	@./jobqueue
