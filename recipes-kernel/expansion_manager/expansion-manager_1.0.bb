#

SUMMARY = "Kernel module to deal with device tree overlays loading"
LICENSE = "GPLv1"
LIC_FILES_CHKSUM = "file://COPYING;md5=6d047b99124d03afb6c41a6ad8ef4a6c"

inherit module

SRC_URI = " \
    file://COPYING \
    file://Makefile \
    file://expansion_manager.c \
"

S = "${WORKDIR}"
