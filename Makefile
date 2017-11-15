TARGET = abyss
SRCS = src/*.cpp

abyss : 
	g++ -std=c++11 -o $@ $(SRCS)

clean :
	rm abyss