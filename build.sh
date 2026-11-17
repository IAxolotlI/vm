#!/bin/bash
mkdir -p build/
FLAGS="-ggdb -Wall -Wextra -pedantic"
INCLUDE_GME="-Igm -Icommon -Iinterface"
INCLUDE_GASM="-Igasm/assembler -Icommon -Iinterface"
GME_COMMON_DIRS=(
    ./interface/
    ./common/
    ./gm/
)
GASM_COMMON_DIRS=(
    ./interface/
    ./common/
    ./gasm
)
GME_ENTRY="./gme/gme.c"
GASM_ENTRY="./gasm/gasm.c"

expand_sources() {
    local dep
    for dep in "$@"; do
        if [ -d "$dep" ]; then
            find "$dep" -type f -name '*.c'
        elif [ -e "$dep" ]; then
            echo "$dep"
        fi
    done
}

expand_deps() {
    local dep
    for dep in "$@"; do
        if [ -d "$dep" ]; then
            find "$dep" -type f \( -name '*.c' -o -name '*.h' \)
        elif [ -e "$dep" ]; then
            echo "$dep"
        fi
    done
}

needs_rebuild() {
    local target="$1"
    shift

    if [ ! -e "$target" ]; then
        return 0
    fi

    local source
    while IFS= read -r source; do
        [ -z "$source" ] && continue
        if [ "$source" -nt "$target" ]; then
            return 0
        fi
    done < <(expand_deps "$@")

    return 1
}

collect_sources() {
    local entry="$1"
    shift
    local src
    echo "$entry"
    while IFS= read -r src; do
        [ -z "$src" ] && continue
        [ "$src" = "$entry" ] && continue
        echo "$src"
    done < <(expand_sources "$@")
}

build_all() {
    local gme_sources gasm_sources

    if needs_rebuild ./build/gme "${GME_COMMON_DIRS[@]}" "$GME_ENTRY"; then
        mapfile -t gme_sources < <(collect_sources "$GME_ENTRY" "${GME_COMMON_DIRS[@]}")
        set -e
        echo "Building gme: ${gme_sources[*]}"
        gcc $INCLUDE_GME $FLAGS "${gme_sources[@]}" -o ./build/gme
        set +e
    fi

    if needs_rebuild ./build/gasm "${GASM_COMMON_DIRS[@]}" "$GASM_ENTRY"; then
        mapfile -t gasm_sources < <(collect_sources "$GASM_ENTRY" "${GASM_COMMON_DIRS[@]}")
        set -e
        echo "Building gasm: ${gasm_sources[*]}"
        gcc $INCLUDE_GASM $FLAGS "${gasm_sources[@]}" -o ./build/gasm
        set +e
    fi
}

run_examples() {
    local src name
    for src in examples/*.gasm; do
        name="$(basename "$src" .gasm)"
        set +e
        echo "$name example: "
        ./build/gasm "$src" -o "build/$name.gm"
        ./build/gme "build/$name.gm"
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
        --build)    do_build=1 ;;
        --examples) do_examples=1 ;;
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
