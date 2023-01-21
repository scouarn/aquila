#!/usr/bin/sh
CHAR_DELAY=30
LINE_DELAY=30
PORT=/dev/ttyACM0

picocom -q --omap lfcr \
        --send-cmd "ascii-xfr -s -c $CHAR_DELAY -l $LINE_DELAY" $PORT
