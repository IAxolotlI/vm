#!/bin/bash
mkdir -p build/
FLAGS="-ggdb -Wall -Wextra"
INCLUDE="-Icommon -Igm"

needs_rebuild() {
    local target="$1"
    shift

    if [ ! -e "$target" ]; then
        return 0
    fi

    local source
    for source in "$@"; do
        if [ "$source" -nt "$target" ]; then
            return 0
        fi
    done

    return 1
}

GME_DEPS=(
    ./gm/gme.c
    ./gm/gm.c
    ./gm/gm.h
    ./gm/opcodes.h
    ./common/sv.h
    ./common/logger.h
)

GASM_DEPS=(
    ./gasm/gasm.c
    ./gasm/compiler.c
    ./gasm/compiler.h
    ./gasm/emit.h
    ./gm/opcodes.h
    ./common/sv.h
    ./common/logger.h
    ./common/container.h
)

build_all() {
    if needs_rebuild ./build/gme "${GME_DEPS[@]}"; then
        set -e
        gcc $INCLUDE $FLAGS ./gm/gme.c ./gm/gm.c -o ./build/gme
        set +e
    fi

    if needs_rebuild ./build/gasm "${GASM_DEPS[@]}"; then
        set -e
        gcc $INCLUDE $FLAGS ./gasm/gasm.c ./gasm/compiler.c -o ./build/gasm
        set +e
    fi
}

run_examples() {
    local src name
    for src in examples/*.gasm; do
        name="$(basename "$src" .gasm)"
        set -e
        echo "$name example: "
        ./build/gasm "$src" -o "build/$name.gm"
        ./build/gme "build/$name.gm"
        set +e
    done
}

if [ $# -eq 0 ]; then
    echo "Usage: $0 [--build] [--examples]"
    exit 1
fi

do_build=0
do_examples=0

while [ $# -gt 0 ]; do
    case "$1" in
        --build)
            do_build=1
            ;;
        --examples)
            do_examples=1
            ;;
        -h|--help)
            echo "Usage: $0 [--build] [--examples]"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            echo "Usage: $0 [--build] [--examples]" >&2
            exit 1
            ;;
    esac
    shift
done

[ "$do_build" -eq 1 ] && build_all
[ "$do_examples" -eq 1 ] && run_examples
exit 0
