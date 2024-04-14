CC 			= g++
CFLAGS		= -Wall -Wno-parentheses -I/usr/include -std=c++20 -O3 -g -flto
LDFLAGS		= -flto -lm -lraylib -lpthread

DIR_SRC     = ./src
DIR_BUILD   = ./bin

TARGET      = neoterm

SRCS        = $(wildcard $(DIR_SRC)/*.cpp)
OBJS        = $(filter-out $(DIR_BUILD)/main.o, $(patsubst $(DIR_SRC)/%.cpp, $(DIR_BUILD)/%.o, $(SRCS)))

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS) $(DIR_BUILD)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(DIR_BUILD)/%.o: $(DIR_SRC)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(DIR_BUILD)/*.o
