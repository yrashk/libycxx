# Load libycxx's toolchains into the current fish session:
#
#   source tools/toolchain/activate.fish [--provision]
#
# The fish counterpart of activate.sh (see there for the variables it exports). Defines
# `ycxx-unload`, which restores everything it changed and removes itself.

set -l _ycxx_here (status dirname)
set _ycxx_here (cd $_ycxx_here; and pwd)
set -l _ycxx_root (cd $_ycxx_here/../..; and pwd)
set -l _ycxx_data $XDG_DATA_HOME
test -n "$_ycxx_data"; or set _ycxx_data $HOME/.local/share
set -l _ycxx_conf $_ycxx_data/ycxx/toolchains/toolchains.env
test -n "$YCXX_TOOLCHAINS"; and set _ycxx_conf $YCXX_TOOLCHAINS/toolchains.env

if test "$argv[1]" = --provision
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
    set -gx PATH $prepend $PATH

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
end
