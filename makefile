.PHONY: all clean install

plugin_directory = lv2/horst-plugins.lv2
plugin_names = worker-test state-test
plugins = $(plugin_names:%=$(plugin_directory)/%.so)
# plugins = lv2/horst-plugins.lv2/worker-test.so lv2/horst-plugins.lv2/state-test.so

all: $(plugins) src/horst.so

HORST_HEADERS = $(wildcard src/include/horst/*.h)
HORST_SOURCES = ${wildcard src/*.cc}
HORST_OBJECTS = ${HORST_SOURCES:.cc=.o}

# OPTIMIZATION_FLAGS = -O2
OPTIMIZATION_FLAGS = -O3 -mtune=native -march=native -mcpu=native 

ifeq ($(DEBUG),1)
OPTIMIZATION_FLAGS = -O1 -g -pg -DHORST_DEBUG -fstack-protector -fstack-protector-strong -fstack-protector-all -fstack-protector-explicit
endif

PYTHON_CXXFLAGS = `python3 -m pybind11 --includes` `python3-config --cflags`  
PYTHON_LDFLAGS = `python3-config --ldflags --embed` 

COMMON_CXXFLAGS = -std=c++20 -Wall -pedantic

CXXFLAGS += -fPIC -std=c++20 -Isrc/include -Wall -pedantic `pkg-config lilv-0 lv2 jack --cflags` -pthread $(OPTIMIZATION_FLAGS) 
LDFLAGS += `pkg-config lilv-0 jack --libs` -latomic -pthread

src/horst.so: src/horst_python.cc $(HORST_HEADERS) makefile
	g++ -shared -o $@ $(CXXFLAGS) $(PYTHON_CXXFLAGS) $< $(LDFLAGS) $(PYTHON_LDFLAGS)

# $(plugins): %.so %.cc
# 	g++ -shared -o $@ $<

$(plugin_directory)/%.so: $(plugin_directory)/%.cc makefile
	g++ $(COMMON_CXXFLAGS) -shared -o $@ $<

clean:
	rm -f src/*.o src/horst.so src/horst_internal.so src/test_horst src/test_horst2 src/horst_cli

PREFIX ?= /usr/local

install: all
	install -d $(PREFIX)/lib/horst
	install -d $(PREFIX)/bin
	install src/horst.so $(PREFIX)/lib/horst
	install src/horsting.py $(PREFIX)/lib/horst
	install src/horst_cli $(PREFIX)/bin
