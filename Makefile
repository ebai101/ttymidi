all:
	gcc src/ttymidi.c -o ttymidi -lasound
debug:
	gcc src/ttymidi.c -o ttymidi -g -lasound
install:
	mkdir -p $(DESTDIR)/bin
	cp ttymidi $(DESTDIR)/bin
uninstall:
	rm $(DESTDIR)/bin/ttymidi
