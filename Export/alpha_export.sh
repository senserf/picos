#!/bin/sh
echo "exporting to ../../__ALPHA__"
cd ../Apps
echo "Cleaning up PICOS"
./cleanup -p
cd ../..
rm -rf __ALPHA__
mkdir __ALPHA__
echo "Repacking PIP"
tar cf - PIP | gzip -9 > __pip.tgz
cd __ALPHA__
mkdir PICOS
mkdir PICOS/PicOS
zcat ../__pip.tgz | tar -xf -
cd PIP
rm -rf .git
cd ../..
rm -rf __pip.tgz
echo "Trimming PICOS"
PICOS/Scripts/pxport PICOS/Export/alpha.exp PICOS __ALPHA__/PICOS
echo "Done"
