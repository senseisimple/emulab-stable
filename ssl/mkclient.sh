#!/bin/sh

prefix=$1

#
# Create a client side private key and certificate request.
#
openssl req -new -config ${prefix}.cnf \
	-keyout ${prefix}_key.pem -out ${prefix}_req.pem
	
#
# Sign the client cert request, creating a client certificate.
#
openssl ca -batch -policy policy_match -config ca.cnf \
	-out ${prefix}_cert.pem \
        -cert cacert.pem -keyfile cakey.pem \
	-infiles ${prefix}_req.pem

#
# Combine the key and the certificate into one file which is installed
# on each remote node and used by tmcc. Installed on boss too so
# we can test tmcc there.
#
cat ${prefix}_key.pem ${prefix}_cert.pem > ${prefix}.pem

exit 0
