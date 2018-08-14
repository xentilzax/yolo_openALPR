#!/bin/bash

set -eux

#
# Собирает DEB-пакет для inex license plate recognazer 
#
COMPANY="inex"
NAME="lprecognizer"
BRANCH=${1:-"master"}
VERSION="1.0"
DESCRIPTION="License plate recognizer"
WORKSPACE=$PWD

echo "branch = $BRANCH"

git checkout $BRANCH

source ./build.sh

RELEASE_DIR=$PWD/build 

# Считываем метаданные приложения
# NAME, VERSION, DESCRIPTION
#source "$SOURCE_DIR/head_detection/application/queue_detection_application/build/metadata.sh"

if [ "$BRANCH" = "master" ]; then   
  PACKEGE_NAME="$COMPANY-$NAME"
else 
  PACKEGE_NAME="$COMPANY-$NAME-$BRANCH"
fi

echo "package name = $PACKEGE_NAME"

fpm -f -t deb -s dir -C "$RELEASE_DIR" \
    --name "$PACKEGE_NAME" \
    --version "$VERSION" \
    --description "$DESCRIPTION" \
    --prefix "/opt/$COMPANY/$NAME" \
    -p $WORKSPACE
