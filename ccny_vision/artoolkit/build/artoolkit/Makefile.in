all:
	(cd lib/SRC;    make -f Makefile)
	(cd util;       make -f Makefile)
	(cd examples;   make -f Makefile)

clean:
	(cd lib/SRC;    make -f Makefile clean)
	(cd util;       make -f Makefile clean)
	(cd examples;   make -f Makefile clean)

allclean:
	(cd lib/SRC;    make -f Makefile allclean)
	(cd util;       make -f Makefile allclean)
	(cd examples;   make -f Makefile allclean)
	rm -f Makefile
	rm -f include/AR/config.h
	rm -f share/artoolkit-config
