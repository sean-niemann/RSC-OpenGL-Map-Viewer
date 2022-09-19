CC = gcc
CFLAGS = -O3 -Wno-deprecated-declarations

BIN = mapview

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

# Windows
ifeq ($(OS), Windows_NT)
	LDFLAGS = -Ilib\glfw\include -Llib\glfw\lib -lglfw3 -lopengl32 -lglu32 -lgdi32
	BIN := $(BIN).exe
# Mac OS
else ifeq ($(shell uname), Darwin)
	LDFLAGS = -lglfw -framework OpenGL
# Linux and others
else
	LDFLAGS = -lGL -lglfw3
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(RM) $(OBJS)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(BIN)
