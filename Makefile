package = tio-agent
version = 1.0.5
tarname = $(package)
distdir = $(tarname)-$(version)

all clean tio-agent:
	cd src && $(MAKE) $@

dist: $(distdir).tar.gz

$(distdir).tar.gz: $(distdir)
	tar chof - $(distdir) | gzip -9 -c > $@
	rm -rf $(distdir)

$(distdir): FORCE
	mkdir -p $(distdir)/src
	cp Makefile $(distdir)
	cp src/Makefile $(distdir)/src
	cp src/die_with_message.c $(distdir)/src
	cp src/read_line.c $(distdir)/src
	cp src/read_line.h $(distdir)/src
	cp src/sample.txt $(distdir)/src
	cp src/tcp_client.c $(distdir)/src
	cp src/tcp_hdr.h $(distdir)/src
	cp src/translate_agent.c $(distdir)/src
	cp src/translate_agent.h $(distdir)/src
	cp src/translate_parser.c $(distdir)/src
	cp src/translate_parser.h $(distdir)/src
	cp src/translate_sio.c $(distdir)/src
	cp src/translate_socket.c $(distdir)/src
	cp src/unix_client.c $(distdir)/src
	cp src/unix_server.c $(distdir)/src
	cp src/rb.c $(distdir)/src
	cp src/libtree.h $(distdir)/src
	cp src/logmsg.c $(distdir)/src
        
FORCE:
	-rm $(distdir).tar.gz > /dev/null 2>&1
	-rm -rf $(distdir) > /dev/null 2>&1
        
.PHONY: FORCE all clean dist
