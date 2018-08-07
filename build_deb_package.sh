#!/bin/bash

set -eux


#apt-get install ruby-dev build-essential
#gem install fpm

#
# Собирает DEB-пакет для inex_LPR 
#
NAME="lpr"
BRANCH=${BRANCH:-master}
VERSION="1.0"
DESCRIPTION="License plate recognizer"
WORKSPACE=$PWD

echo "branch = $BRANCH"
#mkdir -p inex
#cd inex

#git clone https://github.com/xentilzax/yolo_openALPR.git
git checkout $BRANCH

source ./build.sh

RELEASE_DIR=$PWD/lp_recognizer 

# Считываем метаданные приложения
# NAME, VERSION, DESCRIPTION
#source "$SOURCE_DIR/head_detection/application/queue_detection_application/build/metadata.sh"

if ["$BRANCH" == "master"]; then
    PACKEGE_NAME="$NAME"
else
    PACKEGE_NAME="$NAME-$BRANCH"
fi

echo "package name = $PACKEGE_NAME"

rm -rf $PACKEGE_NAME.deb

fpm -f -t deb -s dir -C "$RELEASE_DIR" \
    --name "$PACKEGE_NAME" \
    --version "$VERSION" \
    --description "$DESCRIPTION" \
#    --iteration "$(utils::iteration $SOURCE_DIR $BUILD_NUMBER)" \
#    --after-install "$CWD/postinst.sh" \
    --prefix "/opt/inex/lpr/$NAME" -p $WORKSPACE
