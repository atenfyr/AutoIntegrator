#!/bin/sh
if ! command -v repak >/dev/null 2>&1
then
    echo "repak not found in PATH or current working directory; cannot proceed"
    exit 1
fi

cp "./LICENSE" "./out/LICENSE"
cp "./README.md" "./out/README.md"
cp "./NOTICE.md" "./out/NOTICE.md"
cp "./LICENSE" "./out/mod/LICENSE"
cp "./README.md" "./out/mod/README.md"
cp "./NOTICE.md" "./out/mod/NOTICE.md"

sed -i -E 's/"[0-9]+\.[0-9]+\.[0-9]+"/"'"$(cat ./version.txt)"'"/g' ./out/manifest.json
sed -i -E 's/"[0-9]+\.[0-9]+\.[0-9]+"/"'"$(cat ./version.txt)"'"/g' ./pak/metadata.json

rm -r "./pak/UE4SS"
cp -r "./out/mod" "./pak/UE4SS"

sed -i "0,/LogicMods/s//default/" "./pak/UE4SS/config.txt"

repak pack --version V4 --compression Zlib "./pak" "./out/PAK-FOR-AMLC/000-AutoIntegratorForAMLC-$(cat ./version.txt)_P.pak"
cd "./out/PAK-FOR-AMLC"
rm *.zip
zip -9 AutoIntegratorForAMLC.zip *.pak
rm *.pak

cd ..
zip -9 -r ../atenfyr-AutoIntegrator-$(cat ../version.txt).zip *

echo "All done!"
