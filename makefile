.PHONY all:

all:
	clang++ -Werror -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -std=c++11 *.cpp

tptest:
	clang++ -Werror -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -std=c++11 main2.cpp

clean:
	echo clean
	$(info 3)


				