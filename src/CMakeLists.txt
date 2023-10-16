# libodiosacd.so

set(SOURCES
    decoder/strdata.c
    decoder/acdata.c
    decoder/codedtable.c
    decoder/framereader.c
    decoder/decoderbase.c
    decoder/decoder.c
    converter/pcmfilter.c
    converter/dsdfilter.c
    converter/filtersetup.c
    converter/converterbase.c
    converter/converter.c
    reader/media.c
    reader/disc.c
    reader/dff.c
    reader/dsf.c
    libodiosacd.c
)

add_library ("odiosacd" SHARED ${SOURCES})
set_target_properties ("odiosacd" PROPERTIES VERSION 1.0.0 SOVERSION 1)
target_link_libraries ("odiosacd" m)
install (TARGETS "odiosacd" LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}")

# reader/disc.h
# reader/media.h
# reader/sacd.h
# libodiosacd.h

set(HEADERS
    reader/disc.h
    reader/media.h
    reader/sacd.h
    libodiosacd.h
)

foreach (FILE ${HEADERS})
    get_filename_component (DIR "${FILE}" DIRECTORY)
    install (FILES "${FILE}" DESTINATION "${CMAKE_INSTALL_FULL_INCLUDEDIR}/libodiosacd/${DIR}")
endforeach()
