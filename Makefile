cpp_version: main.cpp
	c++ -std=c++17 -Wall -Wextra -g -O2 -o cpp_version main.cpp

c_version: main.c
	cc -std=c11 -Wall -Wextra -g -O2 -o c_version main.c

c_optimized: optimized.c
	cc -std=c11 -Wall -Wextra -g -O3 -march=native -o optimized optimized.c
