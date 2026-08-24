#!/bin/bash ../install.sh

NAME='vim'
VERSION='9.2.0538'
DOWNLOAD_URL="https://github.com/vim/vim/archive/refs/tags/v$VERSION.tar.gz#8a1095e157dddb4673aa7b01233f02c4db109b1e5fe562d244cb2caed846f957"
DEPENDENCIES=('ncurses')
CONFIGURE_OPTIONS=(
	'--with-tlib=ncurses'
	'--disable-nls'
	'vim_cv_toupper_broken=no'
	'vim_cv_terminfo=yes'
	'vim_cv_tgetent=yes'
	'vim_cv_getcwd_broken=no'
	'vim_cv_timer_create_works=no'
	'vim_cv_stat_ignores_slash=yes'
	'vim_cv_memmove_handles_overlap=yes'
)

post_install() {
	mkdir -p "$DESTDIR/etc/profile.d"
	echo 'export EDITOR=vim' > "$DESTDIR/etc/profile.d/vim.sh"
}
