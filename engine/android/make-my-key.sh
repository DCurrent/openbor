#!/bin/bash
#
# Make keystore and create keystore.properties file for gradle build.
#
# Execute the script with
# ./make-my-key.sh password [alias-name] 
#
# - password - password to be used for both store and key password
# - alias-name - (optional) if not set, alias name will be default to "alias_name"

KEYNAME="my-key.jks"

# otherwise set alias name to default
if [ -z "$1" ]; then
  ALIAS_NAME="alias_name"
# check if user inputs any alias name
else
  ALIAS_NAME="$1"
fi

# geneate the keystore file
keytool -genkey -v -keystore "${KEYNAME}" -alias "${ALIAS_NAME}" -keyalg RSA -keysize 2048 -validity 10000

# 
