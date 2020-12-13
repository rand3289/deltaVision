LDFLAGS=-lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs
CFLAGS=-g -Wall -std=c++11 -I /usr/include/opencv4
CC=g++
EXE=delta
SOURCES=delta.cpp
HEADERS=$(SOURCES:.cpp=.h)
OBJECTS=$(SOURCES:.cpp=.o)

all: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXE) $(LDFLAGS)

.cpp.o: $(SOURCES) $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	/bin/rm -f *.o $(EXE)
