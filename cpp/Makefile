CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

dbms: dbms.cpp main.cpp dbms.h
	$(CXX) $(CXXFLAGS) dbms.cpp main.cpp -o dbms

clean:
	rm -f dbms demo.data
