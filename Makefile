SOURCES=template_input.cpp template_parser.cpp template_data.cpp template_engine.cpp  example.cpp
EXEC1=example

CXXFLAGS=-std=c++17
FLAGS=-Wall
LDFLAGS=
CPPFLAGS=$(CFLAGS) $(FLAGS)

OBJECTS=$(SOURCES:.cpp=.o)
all: $(EXEC1)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(SOURCES:.cpp=.d)
endif

clean:
	rm -f $(EXEC1) $(OBJECTS) $(SOURCES:.cpp=.d)

%.d: %.cpp
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MT"$@" -MT"$*.o" -o $@ $<  2> /dev/null

$(EXEC1): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

