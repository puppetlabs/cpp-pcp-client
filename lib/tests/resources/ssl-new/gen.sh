#!/usr/bin/env bash

set -ex
# Clean up any left over data from before
# This script needs v3.ext
rm *pem inventory* *csr *srl
# Setup min required files
touch "inventory"
## Min config
# printf "%s\n" "database = inventory" "default_md = md5" > "conf.cnf"

openssl genrsa -aes128 -out ca_key.pem -passout pass:my_password 2048

openssl req -new -key ca_key.pem -x509 -days 1000 -out ca.pem -subj /CN=ca -passin pass:my_password

openssl ca -name ca -config <(echo database = inventory) -keyfile ca_key.pem -passin pass:my_password -cert ca.pem -md sha256 -gencrl -crldays 1000 -out crl_empty.pem

openssl req -new -sha256 -nodes -out good_cert.csr -newkey rsa:2048 -keyout good_key.pem -subj /CN=good

openssl req -new -sha256 -nodes -out bad_cert.csr -newkey rsa:2048 -keyout bad_key.pem -subj /CN=bad

openssl x509 -req -in good_cert.csr -CA ca.pem -CAkey ca_key.pem -CAcreateserial -out good_cert.pem -passin pass:my_password -days 999 -sha256 -extfile v3.ext

openssl x509 -req -in bad_cert.csr -CA ca.pem -CAkey ca_key.pem -CAcreateserial -out bad_cert.pem -passin pass:my_password -days 999 -sha256 -extfile v3.ext

openssl ca -name ca -config <(echo database = inventory) -keyfile ca_key.pem -passin pass:my_password -cert ca.pem -md sha256 -revoke bad_cert.pem

openssl ca -name ca -config <(echo database = inventory) -keyfile ca_key.pem -passin pass:my_password -cert ca.pem -md sha256 -gencrl -crldays 1000 -out crl_bad_revoked.pem