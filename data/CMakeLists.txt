# libodiosacd.pc

configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/libodiosacd.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/libodiosacd.pc" @ONLY)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/libodiosacd.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
