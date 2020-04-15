#!/usr/bin/env ksh93

srcRoot=$1

publicTmpl=$srcRoot/tools/clang-tidy-public.tpl

typeset -T Package=(
    typeset name prefix dir

	function writeHdrTidyFile
	{
		sed "s/%PREFIX%/${_.prefix}/g" $publicTmpl \
			> $srcRoot/${_.dir}/hdr/${_.prefix}/.clang-tidy
	}

	function writeFiles
	{
		_.writeHdrTidyFile
		print "Generated Clang-Tidy files for ${_.name}"
	}
)

Package -a packages

function addPackage
{
	Package newPkg=(name="$1" prefix="$2" dir="$3")
	packages+=(newPkg)
}

addPackage LibS16 S16 lib/s16
addPackage LibPBus PBus lib/PBus

for i in ${!packages[@]}
do
	packages[${i}].writeFiles
done