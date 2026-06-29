#!/bin/bash
# Sorts and renames builds that were uploaded via sftp to make them available for download

# Examples of currently uploaded build file names
# 16379399612-vcmi-branch-develop-7fcb024-intel.dmg
# 16379399612-vcmi-branch-develop-7fcb024-arm.dmg
# 16379399612-vcmi-branch-develop-7fcb024.ipa
# 16379399612-vcmi-branch-develop-7fcb024.exe
# 16379399612-vcmi-branch-develop-7fcb024-armeabi-v7a.apk
# 16379399612-vcmi-branch-develop-7fcb024-arm64-v8a.apk

SOURCE_BASE_DIRECTORY="/home/downloader/tmp"
TARGET_BASE_DIRECTORY="/home/downloader/www"

for fullpath in "${SOURCE_BASE_DIRECTORY}"/*.*
do
	echo "Full path: ${fullpath}"
	filename=${fullpath##*/}
	echo "Filename: ${filename}"

	if [[ $filename =~ ^([0-9^\-]+)\-vcmi\-(branch|PR)\-([^-]+)\-([a-f0-9]+)(-[^.]+)? ]]
	then
		jobidentifier=${BASH_REMATCH[1]} # unique Github job identifier, unused
		category=${BASH_REMATCH[2]}      # branch or PR. Only branches are uploaded right now
		branchname=${BASH_REMATCH[3]}    # name of branch or PR number
		commitsha=${BASH_REMATCH[4]}     # commit sha1

		architecture=${BASH_REMATCH[5]}  # optional, name of system architecture (x86/x64/arm/etc)
		architecture=${architecture:1}   # remove leading dash

		extension=${filename##*.}

		echo "Github Job Id: ${jobidentifier}"
		echo "Category: ${category}"
		echo "Branch name or PR number: ${branchname}"
		echo "Commit SHA1:  ${commitsha}"
		echo "Architecture: ${architecture}"
		echo "Extension: ${extension}"

		if [[ "$category" != "branch" && "$category" != "PR" ]]
		then
			echo "Unknown category :-("
			continue
		fi

		if [[ "$extension" == "exe" ]]
		then
			system="windows"
		elif [[ "$extension" == "dmg" ]]
		then
			system="macos"
		elif [[ "$extension" == "ipa" ]]
		then
			system="ios"
		elif [[ "$extension" == "apk" ]]
		then
			system="android"
		elif [[ "$extension" == "AppImage" ]]
		then
			system="linux"
		else
			echo "Unknown extension :-("
			continue
		fi

		echo "System: ${system}"

		if [[ "$architecture" ]]
		then
			targetdirectory="${TARGET_BASE_DIRECTORY}/${category}/${branchname}/${system}-${architecture}"
		else
			targetdirectory="${TARGET_BASE_DIRECTORY}/${category}/${branchname}/${system}"
		fi

		targetname="VCMI-${category}-${branchname}-${commitsha:0:7}.${extension}"
		echo "Target Directory: $targetdirectory"
		echo "Target Name: $targetname"

		mkdir -p "$targetdirectory"
		mv -n "${fullpath}" "$targetdirectory/$targetname"
	fi
done
