#!/bin/bash

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

function convertAndDiff() {

	./x2ttester ./config_$conversionType.txt > /dev/null
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
		cd ..
		diff -r $file.$inFormat ../out_${conversionType}_snap/$file.$inFormat/
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
