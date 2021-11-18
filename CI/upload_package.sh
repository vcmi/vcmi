#!/bin/sh
if [ -z "$DEPLOY_RSA" ];
then
	# Due to security measures travis not expose encryption keys for PR from forks
	echo "Build generation is skipped for forks"
	exit 0
fi

echo "$DEPLOY_RSA" > /tmp/deploy_rsa
chmod 600 /tmp/deploy_rsa

eval "$(ssh-agent -s)"
ssh-add /tmp/deploy_rsa

sftp -r -o StrictHostKeyChecking=no travis@beholder.vcmi.eu <<< "put $VCMI_PACKAGE_FILE_NAME.$PACKAGE_EXTENSION /incoming/$VCMI_PACKAGE_FILE_NAME.$PACKAGE_EXTENSION"
