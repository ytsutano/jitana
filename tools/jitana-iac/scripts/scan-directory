#!/bin/bash

if [ $# -eq 0 ]; then
    echo $#
    echo "usage: $0 [directory of APK files]"
    exit 1
fi

START_TOTAL=$(date +%s)

rm -rf extracted
mkdir -p extracted
cp $1/* extracted

cd extracted

in_apk_arr=( $(ls *.apk ) )
for f in "${in_apk_arr[@]}"; do
    if [ "$f" != ${f/Google/} ] || [ "$f" = "framework-res.apk" ]; then
        rm "$f"
    else
        file_name=${f/.apk/}
        mkdir $file_name
        unzip "$f" -d $file_name
        (aapt dump xmltree ${f} AndroidManifest.xml > \
                ${file_name}/${file_name}.manifest.xml)
        echo "${file_name} $(pwd)/${file_name}/" >> location.txt
        rm "$f"
    fi
done

cd ..

END_PRE_PROCESS=$(date +%s)
DIFF_PRE_PROCESS=$(($END_PRE_PROCESS - $START_TOTAL ))

START_JITANA_GRAPH=$(date +%s)
./jitana-iac
END_JITANA_GRAPH=$(date +%s)

DIFF_JITANA_GRAPH=$(($END_JITANA_GRAPH - $START_JITANA_GRAPH ))
DIFF_TOTAL=$(($END_JITANA_GRAPH - $START_TOTAL ))

echo "It took $DIFF_PRE_PROCESS seconds for DIFF_PRE_PROCESS "
echo "It took $DIFF_JITANA_GRAPH seconds for JITANA_GRAPH "
echo "It took $DIFF_TOTAL seconds for DIFF_TOTAL "
