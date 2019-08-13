# MDG-217 camera assembly stream format userspace parser V4L2 plugin

May be used with an unpatched kernel, or with a kernel patched with the [UVC kernel patch](https://github.com/digitalloggers/geocam-uvc). Additionally, [configuration binaries](https://github.com/digitalloggers/geocam-bin) are required.

Needs libv4l2, including development headers for build (apt-get install libv4l-dev on Debian and derivatives).

Place the plugin .so built using 'make' into the distribution-specific libv4l plugin directory (/usr/lib/$(dpkg-architecture -qDEB\_HOST\_MULTIARCH)/libv4l/plugins/ on Debian and derivatives).

You may need to LD\_PRELOAD the libv4l/v4l1compat.so library (which should be included into your distribution, e.g. /usr/lib/$(dpkg-architecture -qDEB\_HOST\_MULTIARCH)/libv4l/v4l1compat.so on Debian and derivatives) to have applications' V4L2 requests routed through the plugin system.
