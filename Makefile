CXX      ?= g++
CXXFLAGS ?= -std=c++14 -Wall -Wextra -pedantic -O2
SRC      := main.cpp Inventory.cpp Logistics.cpp Persistence.cpp

erl_hub: $(SRC) $(wildcard *.h)
	$(CXX) $(CXXFLAGS) $(SRC) -o erl_hub

run: erl_hub
	./erl_hub

clean:
	rm -f erl_hub *.bin *.tmp
