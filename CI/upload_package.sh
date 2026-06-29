#!/bin/sh
if [ -z "$BUILDS_UPLOADER_PRIVATE_KEY" ];
then
	# Due to security measures travis not expose encryption keys for PR from forks
	echo "Build generation is skipped for forks"
	exit 0
fi

echo "$BUILDS_UPLOADER_PRIVATE_KEY" > /tmp/deploy_rsa
chmod 600 /tmp/deploy_rsa

eval "$(ssh-agent -s)"
ssh-add /tmp/deploy_rsa

sftp -r -o StrictHostKeyChecking=no uploader@upload.vcmi.eu <<< "put $VCMI_PACKAGE_FILE_NAME.$PACKAGE_EXTENSION /uploads/$VCMI_PACKAGE_FILE_NAME.$PACKAGE_EXTENSION"
