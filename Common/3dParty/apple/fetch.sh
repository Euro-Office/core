#!/bin/bash

# see Common/3dParty/apple/fetch.py

# glm
if [ ! -d glm ]
then
    echo "Fetching glm"
    git clone https://github.com/g-truc/glm.git
    cd glm
    git checkout 33b4a621a697a305bc3a7610d290677b96beb181
    cd ..
fi

# mdds
if [ ! -d mdds ]
then
    echo "Fetching & patching mdds"
    git clone https://github.com/kohei-us/mdds.git
    cd mdds
    git checkout 0783158939c6ce4b0b1b89e345ab983ccb0f0ad0
    git apply ../mdds.patch
    cd ..
fi

# librevenge
if [ ! -d librevenge ]
then
    echo "Fetching & patching librevenge"
    git clone https://github.com/Distrotech/librevenge.git
    cd librevenge
    git checkout becd044b519ab83893ad6398e3cbb499a7f0aaf4
    git apply ../librevenge.patch
    cd ..
fi

# libodfgen
if [ ! -d libodfgen ]
then
    echo "Fetching libodfgen"
    git clone https://github.com/Distrotech/libodfgen.git
    cd libodfgen
    git checkout 8ef8c171ebe3c5daebdce80ee422cf7bb96aa3bc
    cd ..
fi

# libetonyek
if [ ! -d libetonyek ]
then
    echo "Fetching & patching libetonyek"
    git clone https://github.com/LibreOffice/libetonyek.git
    cd libetonyek
    git checkout cb396b4a9453a457469b62a740d8fb933c9442c3
    git apply ../libetonyek.patch
    cd ..
fi

# fetch.py is called with use_gperf = False, so I won't bother with that now.
