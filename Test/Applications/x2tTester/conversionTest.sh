#!/bin/bash

workDir="$1"
exeDir="$2"
exitCode=0

docZipFormats=( docx odt )
docBinFormats=( rtf )
docFormats=(${docZipFormats[@]} ${docBinFormats[@]})
docFiles=( empty medium fonts_and_images )

slidesZipFormats=( odp pptx )
slidesFormats=(${slidesZipFormats[@]})
slidesFiles=( simple )

sheetsZipFormats=( ods xlsx )
sheetsFormats=(${sheetsZipFormats[@]})
sheetsFiles=( spread_basic )

# The internal format sdkjs reads (Editor.bin in a doct/xlst/pptt package) is
# compared as a text dump: see Test/BinaryFormat/README.md. BINFORMAT_DUMPS=0
# skips that, for builds whose Editor.bin differs from the snapshots' (the
# desktop image measures spreadsheet column widths with other fonts).
dumper="$(cd "$(dirname "$0")/../../BinaryFormat" && pwd)/binformat_dump.py"
binformatDumps=${BINFORMAT_DUMPS:-1}
diffExclude=()
if [ "$binformatDumps" != 1 ]
then
	diffExclude=( -x '*.doct.txt' -x '*.xlst.txt' -x '*.pptt.txt' )
fi

cd $workDir

function convertAndDiff() {

	$exeDir/x2ttester ./config_$conversionType.txt > /dev/null
	cd out_$conversionType

	for file in "${files[@]}"
	do
		for inFormat in "${formats[@]}"
		do
			cd $file.$inFormat
			for outFormat in "${zipFormats[@]}"
			do
				unzip $file.$outFormat -d $outFormat > /dev/null 2>&1
				rm $file.$outFormat > /dev/null 2>&1
			done
			if [ -f $file.$internalFormat ]
			then
				if [ "$binformatDumps" = 1 ]
				then
					python3 "$dumper" $file.$internalFormat > $file.$internalFormat.txt || exitCode=1
				fi
				rm $file.$internalFormat
			fi
		cd ..
		diff -r "${diffExclude[@]}" $file.$inFormat ../out_${conversionType}_snap/$file.$inFormat/
		if [ "$?" != 0 ]
		then 
			exitCode=1
		fi
		done
	done

	cd ..

}


zipFormats=(${docZipFormats[@]})
formats=(${docFormats[@]})
files=(${docFiles[@]})
internalFormat=doct
conversionType=word_proc

convertAndDiff

zipFormats=(${slidesZipFormats[@]})
formats=(${slidesFormats[@]})
files=(${slidesFiles[@]})
internalFormat=pptt
conversionType=slides

convertAndDiff

zipFormats=(${sheetsZipFormats[@]})
formats=(${sheetsFormats[@]})
files=(${sheetsFiles[@]})
internalFormat=xlst
conversionType=sheets

convertAndDiff

exit "$exitCode"
