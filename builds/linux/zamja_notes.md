these work, fresh debian build box.

readiness:
    install-prereqs.sh
compile:
    build-cmake.sh
install:
    install-build.sh
debug:
    remake-copy.bbs
    gdb-root.sh
