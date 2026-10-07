# Load libycxx's toolchains into the current fish session:
#
#   source tools/toolchain/activate.fish [--provision] [--use gcc|clang|DIR]
#
# The fish counterpart of activate.sh (see there for the variables it exports, and --use). Defines
# `ycxx-unload`, which restores everything it changed and removes itself.

set -l _ycxx_here (status dirname)
set _ycxx_here (cd $_ycxx_here; and pwd)
set -l _ycxx_root (cd $_ycxx_here/../..; and pwd)
set -l _ycxx_data $XDG_DATA_HOME
test -n "$_ycxx_data"; or set _ycxx_data $HOME/.local/share
set -l _ycxx_conf $_ycxx_data/ycxx/toolchains/toolchains.env
test -n "$YCXX_TOOLCHAINS"; and set _ycxx_conf $YCXX_TOOLCHAINS/toolchains.env

set -l _ycxx_provision
set -l _ycxx_use
set -l _ycxx_i 1
while test $_ycxx_i -le (count $argv)
    switch $argv[$_ycxx_i]
        case --provision
            set _ycxx_provision 1
        case --use
            set _ycxx_i (math $_ycxx_i + 1)
            set _ycxx_use $argv[$_ycxx_i]
        case '--use=*'
            set _ycxx_use (string replace -- --use= '' $argv[$_ycxx_i])
        case '*'
            echo "activate: unknown argument '$argv[$_ycxx_i]' (--provision, --use gcc|clang|DIR)" >&2
            return 2
    end
    set _ycxx_i (math $_ycxx_i + 1)
end
# --use: the libycxx build or installation, with its compiler wrapper.
set -l _ycxx_use_tc
set -l _ycxx_use_pc
if test -n "$_ycxx_use"
    contains -- $_ycxx_use gcc clang; and set _ycxx_use $_ycxx_root/build/$_ycxx_use
    if not test -x $_ycxx_use/bin/ycxx-c++
        echo "activate: --use: no bin/ycxx-c++ in $_ycxx_use (a libycxx build tree, or the prefix of 'cmake --install')" >&2
        return 1
    end
    set _ycxx_use (cd $_ycxx_use; and pwd)
    for _ycxx_f in $_ycxx_use/toolchain.cmake $_ycxx_use/lib/cmake/libycxx/toolchain.cmake $_ycxx_use/lib64/cmake/libycxx/toolchain.cmake
        if test -f $_ycxx_f
            set _ycxx_use_tc $_ycxx_f
            break
        end
    end
    for _ycxx_d in $_ycxx_use/pkgconfig $_ycxx_use/lib/pkgconfig $_ycxx_use/lib64/pkgconfig
        if test -f $_ycxx_d/libycxx.pc
            set _ycxx_use_pc $_ycxx_d
            break
        end
    end
end

if test -n "$_ycxx_provision"
    if not $_ycxx_here/provision
        echo "activate: provisioning failed" >&2
        return 1
    end
else if not test -f $_ycxx_conf
    $_ycxx_here/provision --detect-only
    or echo "activate: some compilers are missing; run 'source $_ycxx_here/activate.fish --provision'" >&2
end

if test -f $_ycxx_conf
    # A second activation first undoes the first.
    functions -q ycxx-unload; and ycxx-unload

    set -g _ycxx_saved_vars PATH SDKROOT YCXX_SDKROOT YCXX_ROOT YCXX_GCC_BIN YCXX_GCC YCXX_GXX YCXX_GCC_INSTALL_DIR \
        YCXX_CLANG_BIN YCXX_CLANG YCXX_CLANGXX YCXX_LLD YCXX_LLVM_AR
    test -n "$_ycxx_use"; and set -a _ycxx_saved_vars CXX CC CMAKE_TOOLCHAIN_FILE PKG_CONFIG_PATH YCXX_USE
    for v in $_ycxx_saved_vars
        if set -q $v
            set -g _YCXX_OLD_$v $$v
        else
            set -g _YCXX_OLD_$v __ycxx_unset__
        end
    end

    for line in (string match -r '^YCXX_[A-Z_]+=.*' < $_ycxx_conf)
        set -l kv (string split -m 1 = -- $line)
        set -gx $kv[1] $kv[2]
    end
    set -gx YCXX_ROOT $_ycxx_root
    # macOS: compilers not from Apple find the SDK through SDKROOT.
    if test -n "$YCXX_SDKROOT"; and not set -q SDKROOT
        set -gx SDKROOT $YCXX_SDKROOT
    end
    set -l prepend $_ycxx_root/tools
    test -n "$YCXX_CLANG_BIN"; and set prepend $YCXX_CLANG_BIN $prepend
    test -n "$YCXX_GCC_BIN"; and set prepend $YCXX_GCC_BIN $prepend
    test -n "$_ycxx_use"; and set prepend $_ycxx_use/bin $prepend
    set -gx PATH $prepend $PATH
    if test -n "$_ycxx_use"
        set -gx YCXX_USE $_ycxx_use
        set -gx CXX $_ycxx_use/bin/ycxx-c++
        set -gx CC $_ycxx_use/bin/ycxx-cc
        test -n "$_ycxx_use_tc"; and set -gx CMAKE_TOOLCHAIN_FILE $_ycxx_use_tc
        test -n "$_ycxx_use_pc"; and set -gx PKG_CONFIG_PATH $_ycxx_use_pc $PKG_CONFIG_PATH
    end

    function ycxx-unload --description 'Restore the environment from before activating libycxx toolchains'
        for v in $_ycxx_saved_vars
            set -l old_name _YCXX_OLD_$v
            set -l old $$old_name
            if test "$old" = __ycxx_unset__
                set -e $v
            else
                set -gx $v $old
            end
            set -e $old_name
        end
        set -e _ycxx_saved_vars
        functions -e ycxx-unload
    end

    set -l g $YCXX_GXX
    test -n "$g"; or set g none
    set -l c $YCXX_CLANGXX
    test -n "$c"; or set c none
    echo "libycxx toolchains loaded (GCC: $g, Clang: $c); 'ycxx-unload' restores the environment" >&2
    test -n "$_ycxx_use"; and echo "building against libycxx: CXX=$CXX, CC=$CC, CMAKE_TOOLCHAIN_FILE=$_ycxx_use_tc" >&2
end
