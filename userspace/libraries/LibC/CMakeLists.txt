set(LIBC_SOURCES
	arpa/inet.cpp
	assert.cpp
	complex.cpp
	ctype.cpp
	dirent.cpp
	dlfcn.cpp
	endian.cpp
	environ.cpp
	errno.cpp
	fcntl.cpp
	fenv.cpp
	ftw.cpp
	grp.cpp
	inttypes.cpp
	langinfo.cpp
	libgen.cpp
	libintl.cpp
	locale.cpp
	malloc.cpp
	math.cpp
	netdb.cpp
	poll.cpp
	printf_impl.cpp
	pthread.cpp
	pwd.cpp
	scanf_impl.cpp
	sched.cpp
	setjmp.cpp
	signal.cpp
	stdio.cpp
	stdlib.cpp
	string.cpp
	strings.cpp
	sys/banan-os.cpp
	sys/epoll.cpp
	sys/file.cpp
	sys/ioctl.cpp
	sys/mman.cpp
	sys/resource.cpp
	sys/select.cpp
	sys/socket.cpp
	sys/stat.cpp
	sys/statvfs.cpp
	sys/time.cpp
	sys/uio.cpp
	sys/utsname.cpp
	sys/wait.cpp
	syslog.cpp
	termios.cpp
	time.cpp
	unistd.cpp
	utime.cpp
	wchar-stdio.cpp
	wchar.cpp
	icxxabi.cpp

	arch/${BANAN_ARCH}/setjmp.S
	arch/${BANAN_ARCH}/string.S

	../../../BAN/BAN/Assert.cpp
)

add_library(objlibc OBJECT ${LIBC_SOURCES})
target_compile_definitions(objlibc PRIVATE __arch=${BANAN_ARCH})

target_compile_options(objlibc PRIVATE -O2 -g -Wstack-usage=512 -fno-exceptions -fpic -nolibc)
target_compile_options(objlibc PUBLIC -Wall -Wextra -Werror -Wno-error=stack-usage=)

function(add_crtx crtx)
	add_custom_target(${crtx}
		COMMAND ${CMAKE_CXX_COMPILER} -c -o ${CMAKE_INSTALL_LIBDIR}/${crtx}.o ${CMAKE_CURRENT_SOURCE_DIR}/arch/${BANAN_ARCH}/${crtx}.S
	)
	add_dependencies(objlibc ${crtx})
endfunction()

add_crtx(crt0)
add_crtx(crti)
add_crtx(crtn)

banan_include_headers(objlibc ban)
banan_include_headers(objlibc kernel)
banan_include_headers(objlibc libelf)

banan_install_headers(objlibc)

add_library(libc-static STATIC $<TARGET_OBJECTS:objlibc>)
add_library(libc-shared SHARED $<TARGET_OBJECTS:objlibc>)

target_link_options(libc-static PRIVATE -nostdlib)
target_link_libraries(libc-static PRIVATE -lgcc)

target_link_options(libc-shared PRIVATE -nostdlib)
target_link_libraries(libc-shared PRIVATE -lgcc)

install(TARGETS libc-static OPTIONAL)
install(TARGETS libc-shared OPTIONAL)

set_target_properties(libc-static PROPERTIES OUTPUT_NAME libc)
set_target_properties(libc-shared PROPERTIES OUTPUT_NAME libc)

# set SONAME as cmake doesn't set it for some reason??
set_target_properties(libc-shared PROPERTIES LINK_FLAGS "-Wl,-soname,libc.so")

add_library(libc ALIAS libc-shared)

execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libgcc_s.so OUTPUT_VARIABLE LIBGCC_S_LINK OUTPUT_STRIP_TRAILING_WHITESPACE)
file(REAL_PATH ${LIBGCC_S_LINK} LIBGCC_S)
install(FILES ${LIBGCC_S} DESTINATION ${CMAKE_INSTALL_LIBDIR} RENAME libgcc_s.so
	PERMISSIONS
		OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ             GROUP_EXECUTE
		WORLD_READ             WORLD_EXECUTE
)
