#!/bin/sh
if [ ! -z "$TRAVIS_JOB_ID" ];
then
	echo "Using Travis environment variables!"
	TMP_JOBID="$TRAVIS_JOB_ID"
	TMP_BRANCH="$TRAVIS_BRANCH"
	TMP_PRID="$TRAVIS_PULL_REQUEST"
	TMP_COMMIT="$TRAVIS_COMMIT"
elif [ ! -z "${APPVEYOR_JOB_ID}" ];
then
	echo "Using AppVeyor environment variables!"
	TMP_JOBID=$(echo "$APPVEYOR_JOB_ID" | sed 's/[^[:digit:]]\+//g')
	TMP_BRANCH="$APPVEYOR_REPO_BRANCH"
	TMP_PRID="$APPVEYOR_PULL_REQUEST_NUMBER"
	TMP_COMMIT="$APPVEYOR_REPO_COMMIT"
elif [ ! -z "${GITHUB_RUN_ID}" ];
then
	echo "Using Github environment variables!"
	TMP_JOBID="$GITHUB_RUN_ID"
	TMP_BRANCH=$(basename ${GITHUB_REF#refs/heads/})
	TMP_PRID="$PULL_REQUEST"
	TMP_COMMIT=$(git rev-parse --short "$GITHUB_SHA")
else
	echo "No Travir or AppVeyor environment variables found!"
	exit
fi

VCMI_PACKAGE_FILE_NAME="${TMP_JOBID}-vcmi"
VCMI_PACKAGE_NAME_SUFFIX=""
VCMI_PACKAGE_GITVERSION="ON"
if [ -z "$TMP_PRID" ] || [ "$TMP_PRID" == "false" ];
then
	branch_name=$(echo "$TMP_BRANCH" | sed 's/[^[:alnum:]]\+/_/g')
	VCMI_PACKAGE_FILE_NAME="${VCMI_PACKAGE_FILE_NAME}-branch-${branch_name}-${TMP_COMMIT}"
	if [ "${branch_name}" != "master" ];
	then
		VCMI_PACKAGE_NAME_SUFFIX="branch ${branch_name}"
	else
		VCMI_PACKAGE_GITVERSION="OFF"
	fi
else
	VCMI_PACKAGE_FILE_NAME="${VCMI_PACKAGE_FILE_NAME}-PR-${TMP_PRID}-${TMP_COMMIT}"
	VCMI_PACKAGE_NAME_SUFFIX="PR ${TMP_PRID}"
fi

if [ "${VCMI_PACKAGE_NAME_SUFFIX}" != "" ];
then
	VCMI_PACKAGE_NAME_SUFFIX="(${VCMI_PACKAGE_NAME_SUFFIX})"
fi

export VCMI_PACKAGE_FILE_NAME
export VCMI_PACKAGE_NAME_SUFFIX
export VCMI_PACKAGE_GITVERSION