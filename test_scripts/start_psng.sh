#!/bin/bash
# This script starts PeerStreamer-NG

################################################################################
### SCRIPT CONFIGURATION START
################################################################################

# PeerStreamer executable full path
PSNGBASEDIR=${HOME}/peerstreamer-src
PSNGBIN=peerstreamer-ng

# Channels file name (We assume this file is in /tmp directory)
CHFILE="channels.csv"

# Interface PeerStreamer-NG binds to
PSNGIFACE="lo"

################################################################################
### SCRIPT CONFIGURATION STOP
################################################################################

if [ ! -f /tmp/${CHFILE} ]
then
    touch /tmp/${CHFILE}
fi

cd ${PSNGBASEDIR}
./${PSNGBIN} -c /tmp/${CHFILE} -s "iface=${PSNGIFACE}"
cd -
