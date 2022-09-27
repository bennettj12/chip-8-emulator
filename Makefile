all:
	g++ -I source/include -L source/lib -o main source/*.cpp -lmingw32 -lSDL2main -lSDL2