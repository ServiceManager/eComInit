#!/usr/bin/env ksh93

bindir=$1
shift

set -A internalsrcs
set -A internalhdrs
set -A publichdrs

#
# Extract -I and -D directives from fakehdr target
#
hdrcompflags=$(grep -m 1 "fakehdr/fakehdr.c" compile_commands.json \
    | tr -cs '[:graph:]_' '[\n*]' \
    | grep -e "\-I" -e "\-D" \
    | tr '\n' ' ')

function printBanner
{
    printf "\n********\n -- $1: --\n********\n"
}

function runClangTidy
{
    #
    # Among its many other omissions is a flag to force coloured output from
    # clang-tidy, even if LLVM thinks we don't want it. So let's trick it thus:
    #
    LD_PRELOAD="$bindir"/libfakeisatty.so \
          clang-tidy --quiet -p $bindir $@ 2>&1 \
        | grep -v "generated."
}

function processHdr
{
    runClangTidy $1 -- $hdrcompflags
}

for f in $@; do
    if [ -z ${f##*.h} ]
    then
        if [ -z ${f##*/hdr/*} ]
        then
            publichdrs+=($f)
        else
            internalhdrs+=($f)
        fi
    else
        internalsrcs+=($f)
    fi
done

printBanner "Internal Sources"

for f in ${internalsrcs[@]}
do
    runClangTidy $f
done

printBanner "Internal Headers"

for f in ${internalhdrs[@]}
do
    processHdr $f
done

printBanner "Public Headers"
for f in ${publichdrs[@]}
do
    processHdr $f
done

return 0