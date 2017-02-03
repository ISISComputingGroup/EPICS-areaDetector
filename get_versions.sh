#!/bin/sh
baseurl="https://github.com/areaDetector/"

doget() {
    mod=$1
    ver=$2
    wget --quiet ${baseurl}${mod}/archive/${ver}.tar.gz 
    mv -f ${ver}.tar.gz ${mod}-${ver}.tar.gz
}

doget areaDetector R2-5
doget ADAndor R2-4
doget ADAndor3 R2-1
doget ADCSimDetector R2-3
doget ADCore R2-5
doget ADSimDetector R2-3
doget ADSupport R1-0
doget ADURL R2-1
doget ADnED rel1.7_20160509
doget NDDriverStdArrays R1-0
doget pvaDriver R1-0
# ffmpegServer not in release so got latest zip
# ffmpegViewer not in release so got latest zip
