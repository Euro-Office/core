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
# compared as a text dump: see Test/BinaryFormat/README.md.
dumper="$(cd "$(dirname "$0")/../../BinaryFormat" && pwd)/binformat_dump.py"

cd $workDir

# Reads the written archives with a reader that is not core's, see scripts/check_zip.py
zipChecker="$(pwd)/../../../scripts/check_zip.py"
pythonBin="${CORE_PYTHON:-python3}"

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
				# x2t writes nothing for a conversion to the same format
				if [ -f $file.$outFormat ]
				then
					"$pythonBin" "$zipChecker" $file.$outFormat || exitCode=1
				fi
				unzip $file.$outFormat -d $outFormat > /dev/null 2>&1
				rm $file.$outFormat > /dev/null 2>&1
			done
			if [ -f $file.$internalFormat ]
			then
				python3 "$dumper" $file.$internalFormat > $file.$internalFormat.txt || exitCode=1
				rm $file.$internalFormat
			fi
		cd ..
		diff -r $file.$inFormat ../out_${conversionType}_snap/$file.$inFormat/
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
