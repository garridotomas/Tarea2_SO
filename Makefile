CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CXXFLAGS += -pthread
LDLIBS += -pthread

SOURCES := src/rw.cpp src/comunes.cpp src/monitor_rw.cpp src/control_atomico_rw.cpp src/verificador.cpp
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean
all: rw

rw: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	$(RM) $(OBJECTS) $(DEPS) rw

-include $(DEPS)

test-atomico: build/comunes.o build/control_atomico_rw.o tests/prueba_atomico.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDLIBS) -o build/prueba_atomico
	./build/prueba_atomico