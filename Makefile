CXXFLAGS = -std=c++17 -Wall -Wextra -Iengine

MONKEY_SOURCE = engine/object.hpp engine/ast.hpp engine/parser.hpp engine/environment.hpp engine/evaluator.hpp

CLI_HEADER = cli/repl.hpp cli/repl.hpp
CLI_SOURCE = cli/main.cpp

TEST_HEADER = test/test-util.hpp
TEST_SOURCE = test/test-main.cpp test/test-parser.cpp test/test-evaluator.cpp test/test-object.cpp

.PHONY: test clean

all: build/monkey build/test

test:
	./build/test

build/monkey: $(MONKEY_SOURCE) $(CLI_HEADER) $(CLI_SOURCE)
	@if [ ! -e build ]; then mkdir -p build; fi
	$(CXX) $(CXXFLAGS) -o build/monkey $(CLI_SOURCE)

build/test: $(MONKEY_SOURCE) $(TEST_HEADER) $(TEST_SOURCE)
	@if [ ! -e build ]; then mkdir -p build; fi
	$(CXX) $(CXXFLAGS) -o build/test $(TEST_SOURCE)

clean:
	rm -rf build
