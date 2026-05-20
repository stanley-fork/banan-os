set(SOURCES
	main.cpp
)

add_executable(driver-install ${SOURCES})
banan_link_library(driver-install ban)
banan_link_library(driver-install libc)

install(TARGETS driver-install OPTIONAL)
