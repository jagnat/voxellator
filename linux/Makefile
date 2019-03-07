
HDRS := $(wildcard ../src/*.h)
SRCS := $(subst win32_vox.c, ,$(notdir $(wildcard ../src/*.c)))
OBJS := $(SRCS:.c=.o)
CFLAGS := -D_REENTRANT -g

all: $(OBJS)
	clang $(OBJS) -L/usr/lib -lSDL2 -lGLU -lGL -lm -ovox

%.o: ../src/%.c $(HDRS)
	@clang $(CFLAGS) -c ../src/$< -o $@

.PHONY: clean
clean:
	rm -f vox *.o
