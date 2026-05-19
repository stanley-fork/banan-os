set(SOURCES
	main.cpp
)

add_executable(tr ${SOURCES})
banan_link_library(tr ban)
banan_link_library(tr libc)

install(TARGETS tr OPTIONAL)
