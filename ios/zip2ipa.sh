#!/usr/bin/env bash

generatedZip="$1"
if [[ -z "$generatedZip" ]]; then
	echo 'generated zip not provided as param'
	exit 1
fi

mv "$generatedZip" "$(basename "$generatedZip" .zip).ipa"
