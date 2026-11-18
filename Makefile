CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CXXFLAGS += -pthread
LDLIBS += -pthread

SOURCES := src/rw.cpp src/comunes.cpp src/monitor_rw.cpp src/control_atomico_rw.cpp src/verificador.cpp
OBJECTS := $(SOURCES:src/%.cpp=build/%.o)
DEPS := $(OBJECTS:.o=.d)
MONITOR_TEST := build/prueba_monitor
MONITOR_TEST_DEPS := build/prueba_monitor.d

.PHONY: all clean test-monitor
all: rw

rw: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDLIBS) -o $@

build/%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(MONITOR_TEST): tests/prueba_monitor.cpp build/monitor_rw.o build/comunes.o include/monitor_rw.h include/tipos.h include/control_rw.h
	mkdir -p build
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF $(MONITOR_TEST_DEPS) tests/prueba_monitor.cpp build/monitor_rw.o build/comunes.o $(LDLIBS) -o $@

test-monitor: $(MONITOR_TEST)
	./$(MONITOR_TEST)

clean:
	$(RM) $(OBJECTS) $(DEPS) $(MONITOR_TEST) $(MONITOR_TEST).exe $(MONITOR_TEST_DEPS) rw

-include $(DEPS) $(MONITOR_TEST_DEPS)
