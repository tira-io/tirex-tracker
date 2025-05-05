#!/usr/bin/env sh

SCRIPT_DIR=$(dirname "$0")
TRACKER="measure-0.2.12-linux"

TRACKER_METADATA="/tmp/tira-metadata"

if [ -n "$TIRA_OUTPUT_DIR" ]; then
    TRACKER_METADATA="$TIRA_OUTPUT_DIR"
fi

${SCRIPT_DIR}/${TRACKER} --help > /dev/null 2>&1

if [ $? -eq 0 ]; then
    echo "Run command with tirex-tracker"
    mkdir -p $TRACKER_METADATA
    ${SCRIPT_DIR}/${TRACKER} -f irmetadata -o ${TRACKER_METADATA}/tira-ir-metadata.yml "${@}"
else
    echo "Run command without tirex-tracker"
    eval "${@}"
fi
