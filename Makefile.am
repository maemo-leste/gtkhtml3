SUBDIRS =					\
	a11y					\
	gtkhtml					\
	components				\
	art					\
	po

CLEANFILES =					\
	po/.intltool-merge-cache

DISTCLEANFILES =				\
	doltcompile				\
	doltlibtool				\
	intltool-extract			\
	intltool-update				\
	intltool-merge				\
	gtkhtml/libgtkhtml-@GTKHTML_API_VERSION@.pc

MAINTAINERCLEANFILES =				\
	Makefile.in				\
	aclocal.m4				\
	config.guess				\
	config.h.in				\
	config.sub				\
	configure				\
	depcomp					\
	install-sh				\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in			\
	libtool					\
	ltmain.sh				\
	missing					\
	mkinstalldirs				\
	po/Makefile.in.in			\
	po/$(GETTEXT_PACKAGE).pot		\
	stamp.h.in

EXTRA_DIST =					\
	gtkhtml.spec.in				\
	gtkhtml.spec                            \
	gtkhtml-zip.in				\
	COPYING.LIB				\
	BUGS					\
	intltool-extract.in			\
	intltool-update.in			\
	intltool-merge.in

DISTCHECK_CONFIGURE_FLAGS = --with-bonobo-editor
