#!/bin/sh
set -e

[ -d ADApp/ADSrc ] || exit 1

cat <<EOF > configure/CONFIG_SITE.linux-x86_64.Common
WITH_BOOST=NO
WITH_HDF5=NO
XML2_EXTERNAL=YES
XML2_INCLUDE=/usr/include/libxml2
WITH_NETCDF=NO
WITH_NEXUS=NO
WITH_TIFF=NO
WITH_JPEG=NO
WITH_SZIP=NO
WITH_ZLIB=YES
ZLIB_EXTERNAL=YES
WITH_BLOSC=NO
EOF

echo configure/CONFIG_SITE.linux-x86_64.Common
cat configure/CONFIG_SITE.linux-x86_64.Common
