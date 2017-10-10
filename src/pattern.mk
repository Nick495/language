%.o: %.c %.d
	$(CC) $(CFLAGS) -c -o $(OBJ)/$*.o $(CFLAGS) $<
	emcc $(CFLAGS) -c -o $(WEBOBJ)/$*.o $(CFLAGS) $<

%.d: %.c
	@set -e; rm -f $@; \
		$(CC) -M $(CFLAGS) $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$
