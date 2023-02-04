CC ?= gcc

ifndef O
	O = 3
endif

CFLAGS := -g -O$O -Wall -Wno-format-truncation -fpic -Idecoder -Iconverter -Ireader $(CFLAGS)
LDFLAGS += -lpthread -lm -Ldecoder -Lconverter -Lreader -shared -Wl,-soname,libodiosacd.so.1
VPATH = src
OBJECTS = decoder/strdata decoder/acdata decoder/codedtable decoder/framereader decoder/decoderbase decoder/decoder converter/pcmfilter converter/dsdfilter converter/filtersetup converter/converterbase converter/converter reader/media reader/disc reader/dff reader/dsf libodiosacd
SUBDIRS = $(VPATH) $(VPATH)/decoder $(VPATH)/converter $(VPATH)/reader
LIBDIR = lib

ifneq ($(wildcard $(DESTDIR)/usr/lib64),)
    LIBDIR = lib64
endif

.PHONY: all clean install uninstall
all: clean $(OBJECTS) odio-libsacd
decoder/strdata: decoder/strdata.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
decoder/acdata: decoder/acdata.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
decoder/codedtable: decoder/codedtable.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
decoder/framereader: decoder/framereader.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
decoder/decoderbase: decoder/decoderbase.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
decoder/decoder: decoder/decoder.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
converter/pcmfilter: converter/pcmfilter.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
converter/dsdfilter: converter/dsdfilter.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
converter/filtersetup: converter/filtersetup.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
converter/converterbase: converter/converterbase.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
converter/converter: converter/converter.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
reader/media: reader/media.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
reader/disc: reader/disc.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
reader/dff: reader/dff.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
reader/dsf: reader/dsf.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
libodiosacd: libodiosacd.c; $(CC) -c $(CFLAGS) $^ -o $(VPATH)/$@.o
odio-libsacd: $(foreach object, $(OBJECTS), $(VPATH)/$(object).o); $(CC) -o data/usr/lib/libodiosacd.so.1 $^ $(LDFLAGS)

clean:

	$(shell rm -f ./data/usr/lib/libodiosacd.so)
	$(shell rm -f $(foreach librarydir, $(SUBDIRS), $(librarydir)/*.o))

install:

	$(shell install -Dt $(DESTDIR)/usr/$(LIBDIR) ./data/usr/lib/libodiosacd.so.1)
	$(shell ln -sf ./libodiosacd.so.1 $(DESTDIR)/usr/$(LIBDIR)/libodiosacd.so)
	$(shell install -Dt $(DESTDIR)/usr/include/libodiosacd ./src/libodiosacd.h)
	$(shell install -Dt $(DESTDIR)/usr/include/libodiosacd/reader ./src/reader/disc.h)
	$(shell install -Dt $(DESTDIR)/usr/include/libodiosacd/reader ./src/reader/media.h)
	$(shell install -Dt $(DESTDIR)/usr/include/libodiosacd/reader ./src/reader/sacd.h)

ifndef DESTDIR
	ldconfig
endif

uninstall:

	$(shell rm -f $(DESTDIR)/usr/$(LIBDIR)/libodiosacd.so)
	$(shell rm -f $(DESTDIR)/usr/$(LIBDIR)/libodiosacd.so.1)
	$(shell rm -fr $(DESTDIR)/usr/include/libodiosacd)
