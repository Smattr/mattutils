#!/usr/bin/env bash

# Irritatingly there are multiple SSL certificate sources on most Linux
# systems. This script adds a given certificate to the system store and NSS
# (used by Firefox and Chrome).

if [ $# -ne 1 ]; then
    echo "Usage: $0 certificate_file" >&2
    exit 1
fi
CERT=$1
CERT_BASENAME=`basename "${CERT}"`

if [ "$(uname -s)" != "Linux" ]; then
  printf 'This tool is only designed for use on Linux\n' >&2
  exit 1
fi

# Add certificate to the system store.
which openssl &>/dev/null
if [ $? -eq 0 ]; then
    if [ -d /etc/ssl/certs ]; then
        sudo cp "${CERT}" /etc/ssl/certs/
        sudo ln -s "/etc/ssl/certs/${CERT_BASENAME}" /etc/ssl/certs/`openssl x509 -noout -hash < "${CERT}"`.0
    else
        echo "Warning: /etc/ssl/certs doesn't exist; skipping system store" >&2
    fi
else
    echo "Warning: openssl not found; skipping system store" >&2
fi

# Add certificate to NSS.
which certutil &>/dev/null
if [ $? -eq 0 ]; then
    certutil -d sql:${HOME}/.pki/nssdb -A -t "C,," -n "${CERT_BASENAME%.*}" -i "${CERT}"
    # To list NSS certificates:
    #   certutil -d sql:${HOME}/.pki/nssdb -L
    # To delete an NSS certificate:
    #   certutil -d sql:${HOME}/.pki/nssdb -D -n <certificate name>
else
    echo "Warning: certutil (libnss3-tools) not found; skipping NSS" >&2
fi
