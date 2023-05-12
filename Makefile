#Names: Mason Leonard, Brian Huang
#RedIDs: 818030805, 822761804
#Course: CS 480-01
#Assignment 3: Progamming Portion

CXX=g++

CXXFLAGS=-std=c++11 -g

main: 
	g++ -std=c++11 -o pagingwithtlb main.cpp pagetable.cpp tracereader.c output_mode_helpers.c

clean:
	rm -f *.o pagingwithtlb