#!/usr/bin/env bash
set -euo pipefail

APP="${1:-}"

if [[ -z "$APP" ]]; then
    echo "Usage:"
    echo "  $0 /path/to/MyApp.app"
    echo
    echo "Optional environment variables:"
    echo "  LLVM_INSTALL_NAME_TOOL=/path/to/llvm-install-name-tool"
    echo "  FIX_IDS=1"
    exit 1
fi

if [[ ! -d "$APP" ]]; then
    echo "ERROR: app bundle not found:"
    echo "  $APP"
    exit 1
fi

FIX_IDS="${FIX_IDS:-0}"

find_llvm_install_name_tool() {
    if [[ -n "${LLVM_INSTALL_NAME_TOOL:-}" && -x "${LLVM_INSTALL_NAME_TOOL:-}" ]]; then
        printf "%s\n" "$LLVM_INSTALL_NAME_TOOL"
        return 0
    fi

    for candidate in \
        /opt/homebrew/opt/llvm/bin/llvm-install-name-tool \
        /usr/local/opt/llvm/bin/llvm-install-name-tool \
        /opt/homebrew/bin/llvm-install-name-tool \
        /usr/local/bin/llvm-install-name-tool
    do
        if [[ -x "$candidate" ]]; then
            printf "%s\n" "$candidate"
            return 0
        fi
    done

    if command -v llvm-install-name-tool >/dev/null 2>&1; then
        command -v llvm-install-name-tool
        return 0
    fi

    return 1
}

LLVM_INSTALL_NAME_TOOL="$(find_llvm_install_name_tool || true)"

if [[ -z "$LLVM_INSTALL_NAME_TOOL" ]]; then
    echo "ERROR: llvm-install-name-tool not found."
    echo
    echo "Install LLVM with:"
    echo "  brew install llvm"
    echo
    echo "Or specify it explicitly:"
    echo "  LLVM_INSTALL_NAME_TOOL=/path/to/llvm-install-name-tool $0 \"$APP\""
    exit 1
fi

CONTENTS_DIR="$APP/Contents"
FRAMEWORK_DIR="$CONTENTS_DIR/Frameworks"
MACOS_DIR="$CONTENTS_DIR/MacOS"

if [[ ! -d "$FRAMEWORK_DIR" ]]; then
    echo "ERROR: Frameworks directory not found:"
    echo "  $FRAMEWORK_DIR"
    echo
    echo "Run macdeployqt first."
    exit 1
fi

relpath() {
    /usr/bin/python3 -c "import os, sys; print(os.path.relpath(os.path.abspath(sys.argv[2]), os.path.abspath(sys.argv[1])))" "$1" "$2"
}

is_macho() {
    local file="$1"
    otool -L "$file" >/dev/null 2>&1
}

is_direct_main_executable() {
    local file="$1"
    local dir
    dir="$(dirname "$file")"

    # Binaries directly in Contents/MacOS are skipped.
    # For the main executable, @executable_path/../Frameworks is correct.
    [[ "$dir" == "$MACOS_DIR" ]]
}

get_install_id() {
    local file="$1"

    otool -D "$file" 2>/dev/null |
        sed "1d" |
        head -n 1 |
        sed "s/^[[:space:]]*//" || true
}

dependency_is_system() {
    local dep="$1"

    case "$dep" in
        /System/*|/usr/lib/*)
            return 0
            ;;
    esac

    return 1
}

dependency_points_to_framework() {
    local dep="$1"

    [[ "$dep" == *.framework/Versions/*/* ]]
}

dependency_points_to_dylib() {
    local dep="$1"

    [[ "$dep" == *.dylib ]]
}

extract_framework_name() {
    local path="$1"
    local prefix

    case "$path" in
        *.framework/Versions/*/*)
            prefix="${path%%.framework/Versions/*}"
            prefix="${prefix##*/}"
            printf "%s\n" "$prefix"
            return 0
            ;;
    esac

    return 1
}

extract_framework_version() {
    local path="$1"
    local rest

    case "$path" in
        *.framework/Versions/*/*)
            rest="${path#*.framework/Versions/}"
            rest="${rest%%/*}"
            printf "%s\n" "$rest"
            return 0
            ;;
    esac

    return 1
}

framework_exists_in_bundle() {
    local fw="$1"
    local version="${2:-A}"

    [[ -f "$FRAMEWORK_DIR/${fw}.framework/Versions/${version}/${fw}" ]]
}

dylib_exists_in_bundle() {
    local dylib="$1"

    [[ -f "$FRAMEWORK_DIR/$dylib" ]]
}

make_new_dependency() {
    local prefix="$1"
    local old="$2"
    local fw
    local version
    local dylib

    if dependency_is_system "$old"; then
        return 1
    fi

    if dependency_points_to_framework "$old"; then
        fw="$(extract_framework_name "$old" || true)"
        version="$(extract_framework_version "$old" || true)"

        if [[ -z "$fw" ]]; then
            return 1
        fi

        if [[ -z "$version" ]]; then
            version="A"
        fi

        # Rewrite only if the corresponding framework is actually bundled.
        if framework_exists_in_bundle "$fw" "$version"; then
            printf "%s%s.framework/Versions/%s/%s\n" "$prefix" "$fw" "$version" "$fw"
            return 0
        fi

        return 1
    fi

    if dependency_points_to_dylib "$old"; then
        dylib="$(basename "$old")"

        # Rewrite only if the corresponding dylib is actually bundled.
        if dylib_exists_in_bundle "$dylib"; then
            printf "%s%s\n" "$prefix" "$dylib"
            return 0
        fi

        return 1
    fi

    return 1
}

should_try_rewrite_dependency() {
    local dep="$1"

    if dependency_is_system "$dep"; then
        return 1
    fi

    case "$dep" in
        @executable_path/../Frameworks/*)
            return 0
            ;;

        /*)
            if dependency_points_to_framework "$dep" || dependency_points_to_dylib "$dep"; then
                return 0
            fi
            ;;

        *Qt*.framework/Versions/*/*)
            return 0
            ;;

        @rpath/*)
            if dependency_points_to_framework "$dep" || dependency_points_to_dylib "$dep"; then
                return 0
            fi
            ;;

        @loader_path/*)
            if dependency_points_to_framework "$dep" || dependency_points_to_dylib "$dep"; then
                return 0
            fi
            ;;
    esac

    return 1
}

patch_dependencies_for_file() {
    local bin="$1"

    if ! is_macho "$bin"; then
        return 0
    fi

    if is_direct_main_executable "$bin"; then
        return 0
    fi

    local bin_dir
    local rel
    local prefix
    local id
    local deps
    local changed

    bin_dir="$(dirname "$bin")"
    rel="$(relpath "$bin_dir" "$FRAMEWORK_DIR")"

    if [[ "$rel" == "." ]]; then
        prefix="@loader_path/"
    else
        prefix="@loader_path/${rel}/"
    fi

    id="$(get_install_id "$bin")"

    deps="$(
        otool -L "$bin" |
            sed "1d" |
            sed -E "s/^[[:space:]]*([^[:space:]]+).*/\1/" |
            while IFS= read -r dep; do
                [[ -z "$dep" ]] && continue

                # Skip LC_ID_DYLIB / self-ID line.
                if [[ -n "$id" && "$dep" == "$id" ]]; then
                    continue
                fi

                if should_try_rewrite_dependency "$dep"; then
                    printf "%s\n" "$dep"
                fi
            done
    )"

    if [[ -z "$deps" ]]; then
        return 0
    fi

    changed=0

    while IFS= read -r old; do
        [[ -z "$old" ]] && continue

        local new
        new="$(make_new_dependency "$prefix" "$old" || true)"

        if [[ -z "$new" ]]; then
            continue
        fi

        if [[ "$old" == "$new" ]]; then
            continue
        fi

        if [[ "$changed" -eq 0 ]]; then
            echo "Patching dependencies:"
            echo "  $bin"
            echo "  prefix: $prefix"
            changed=1
        fi

        echo "    $old"
        echo "      -> $new"

        codesign --remove-signature "$bin" 2>/dev/null || true
        "$LLVM_INSTALL_NAME_TOOL" -change "$old" "$new" "$bin"
    done <<< "$deps"

    if [[ "$changed" -eq 1 ]]; then
        echo
    fi
}

patch_id_for_file() {
    local bin="$1"

    if [[ "$FIX_IDS" != "1" ]]; then
        return 0
    fi

    if ! is_macho "$bin"; then
        return 0
    fi

    if is_direct_main_executable "$bin"; then
        return 0
    fi

    local id
    local new_id
    local fw
    local version
    local base

    id="$(get_install_id "$bin")"

    if [[ -z "$id" ]]; then
        return 0
    fi

    if [[ "$bin" == *.framework/Versions/*/* ]]; then
        fw="$(extract_framework_name "$bin" || true)"
        version="$(extract_framework_version "$bin" || true)"

        if [[ -z "$fw" ]]; then
            return 0
        fi

        if [[ -z "$version" ]]; then
            version="A"
        fi

        new_id="@rpath/${fw}.framework/Versions/${version}/${fw}"

    elif [[ "$bin" == *.dylib ]]; then
        base="$(basename "$bin")"
        new_id="@rpath/${base}"

    else
        return 0
    fi

    if [[ "$id" == "$new_id" ]]; then
        return 0
    fi

    case "$id" in
        /System/*|/usr/lib/*)
            return 0
            ;;
    esac

    case "$id" in
        @executable_path/../Frameworks/*|@loader_path/*|@rpath/*|/*)
            ;;
        *)
            return 0
            ;;
    esac

    echo "Patching install ID:"
    echo "  $bin"
    echo "    $id"
    echo "      -> $new_id"

    codesign --remove-signature "$bin" 2>/dev/null || true
    "$LLVM_INSTALL_NAME_TOOL" -id "$new_id" "$bin"

    echo
}

echo "Normalizing Mach-O load paths in:"
echo "  $APP"
echo
echo "Using llvm-install-name-tool:"
echo "  $LLVM_INSTALL_NAME_TOOL"
echo
echo "Framework directory:"
echo "  $FRAMEWORK_DIR"
echo
echo "FIX_IDS=$FIX_IDS"
echo

find "$APP" -type f -print0 |
while IFS= read -r -d '' bin; do
    patch_dependencies_for_file "$bin"
done

if [[ "$FIX_IDS" == "1" ]]; then
    find "$APP" -type f -print0 |
    while IFS= read -r -d '' bin; do
        patch_id_for_file "$bin"
    done
fi

echo "Done."
echo
echo "You must now codesign the bundle, for example:"
echo "  codesign --force --deep --sign - \"$APP\""
