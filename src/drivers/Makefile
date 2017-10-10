include ../pattern.mk

all: parse_driver.o print_driver.o

sources = parse_driver.c print_driver.c
include $(sources:.c=.d)

parse_driver.d: parse_driver.c
print_driver.d: print_driver.c

# These are special cased because they don't compile on enscriptem
# due to getchar_unlocked. As such I manually compile it only with $(CC).
parse_driver.o: parse_driver.c parse_driver.d
	$(CC) -c $(CFLAGS) -c -o $(OBJ)/parse_driver.o $(CFLAGS) $<

print_driver.o: print_driver.c print_driver.d
	$(CC) -c $(CFLAGS) -c -o $(OBJ)/print_driver.o $(CFLAGS) $<
