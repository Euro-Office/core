#!/bin/bash

workDir="$1"
exeDir="$2"
exitCode=0

docZipFormats=( docx odt )
docBinFormats=( rtf )
docFormats=(${docZipFormats[@]} ${docBinFormats[@]})
docFiles=( empty medium fonts_and_images )

slidesZipFormats=( odp pptx )
slidesFormats=${slidesZipFormats} 
slidesFiles=( simple )

sheetsZipFormats=( ods xlsx )
sheetsFormats=${sheetsZipFormats} 
sheetsFiles=( spread_basic )

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
					# ZIP_COLLECT_DIR (absolute path) keeps a copy of each archive for scripts/office_smoke.sh,
					# readable by the CI user that runs outside the build container
					if [ -n "$ZIP_COLLECT_DIR" ]
					then
						mkdir -p "$ZIP_COLLECT_DIR" && cp $file.$outFormat "$ZIP_COLLECT_DIR/${conversionType}_$file.$inFormat.$outFormat" && chmod a+r "$ZIP_COLLECT_DIR/${conversionType}_$file.$inFormat.$outFormat"
					fi
				fi
				unzip $file.$outFormat -d $outFormat > /dev/null 2>&1
				rm $file.$outFormat > /dev/null 2>&1
			done
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
conversionType=word_proc

convertAndDiff

zipFormats=(${slidesZipFormats[@]})
formats=(${slidesFormats[@]})
files=(${slidesFiles[@]})
conversionType=slides

convertAndDiff

zipFormats=(${sheetsZipFormats[@]})
formats=(${sheetsFormats[@]})
files=(${sheetsFiles[@]})
conversionType=sheets

convertAndDiff

exit "$exitCode"
