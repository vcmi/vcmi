#!/bin/sh
if [ -z "$encrypted_1d30f79f8582_key" ];
then
	# Due to security measures travis not expose encryption keys for PR from forks
	exit 0
fi
cpack

touch /tmp/deploy_rsa
chmod 600 /tmp/deploy_rsa
openssl aes-256-cbc -K $encrypted_1d30f79f8582_key -iv $encrypted_1d30f79f8582_iv -in $TRAVIS_BUILD_DIR/CI/deploy_rsa.enc -out /tmp/deploy_rsa -d
eval "$(ssh-agent -s)"
ssh-add /tmp/deploy_rsa

sftp -r -o StrictHostKeyChecking=no travis@beholder.vcmi.eu <<< "put $VCMI_PACKAGE_FILE_NAME.dmg /incoming/$VCMI_PACKAGE_FILE_NAME.dmg"
