export SERVER_IP := "192.168.2.5"
export SERVER_PORT := "11922"

setup:
    meson setup builddir

build:
    cd builddir; meson compile

server: build
    ./builddir/server

client: build
    ./builddir/client