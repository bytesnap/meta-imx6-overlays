FILESEXTRAPATHS_prepend := "${META_IMX6_OVERLAYS}/recipes-kernel/linux/files:"

SRC_URI += " \
    file://0001-Fix-dtbs-Makefile.patch \
    file://0002-scripts-dtc-Update-to-overlays-version.patch \
    file://0003-Make-imx-pinctrl-dynamic.patch \
    file://enable-overlays.cfg \
"
