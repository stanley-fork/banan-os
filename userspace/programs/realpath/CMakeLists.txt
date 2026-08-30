set(SOURCES
	main.cpp
)

add_executable(realpath ${SOURCES})
banan_link_library(realpath ban)
banan_link_library(realpath libc)

install(TARGETS realpath OPTIONAL)
