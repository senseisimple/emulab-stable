#!/bin/csh -x
sed -e s/NODE/tbpc$1/g setupmachine-client.tmpl >! /tmp/setupmachine-client.sh
scp /tmp/setupmachine-client.sh 155.99.214.1${1}:/tmp
ssh 155.99.214.1$1 /bin/csh -x /tmp/setupmachine-client.sh
rm -f /tmp/setupmachine-client.sh