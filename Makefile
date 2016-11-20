CFLAGS = -Wall -Werror -Wextra -lm
DEBUG = -g -Wall -Werror -Wextra -pedantic -lm

all: makefolders utf

makefolders:
	mkdir -p bin build

utf: 
	gcc src/utfconverter.c -o ./bin/utf $(CFLAGS)

debug:
	mkdir -p bin build
	gcc src/utfconverter.c -o ./bin/utf $(DEBUG)

.PHONY: clean

clean:
	rm -rf bin build
