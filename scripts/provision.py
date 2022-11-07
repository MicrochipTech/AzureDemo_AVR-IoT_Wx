import os
import datetime
import binascii
import json
import argparse
import cryptography
from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec

import pytz

from serial import Serial
from time import sleep
import sys

ROOT_CA_CERT_FILENAME = "root-ca.crt"
SIGNER_CA_KEY_FILENAME = "signer-ca.key"
SIGNER_CA_CERT_FILENAME = "signer-ca.cer"
DEVICE_CSR_FILENAME = "device-csr.csr"
DEVICE_CERT_FILENAME = "device.cer"

crypto_be = cryptography.hazmat.backends.default_backend()

def device_cert_sn(size, builder):
    """Cert serial number is the SHA256(Subject public key + Encoded dates)"""

    # Get the public key as X and Y integers concatenated
    pub_nums = builder._public_key.public_numbers()
    pubkey =  pub_nums.x.to_bytes(32, byteorder='big', signed=False)
    pubkey += pub_nums.y.to_bytes(32, byteorder='big', signed=False)

    # Get the encoded dates
    expire_years = 0
    enc_dates = bytearray(b'\x00'*3)
    enc_dates[0] = (enc_dates[0] & 0x07) | ((((builder._not_valid_before.year - 2000) & 0x1F) << 3) & 0xFF)
    enc_dates[0] = (enc_dates[0] & 0xF8) | ((((builder._not_valid_before.month) & 0x0F) >> 1) & 0xFF)
    enc_dates[1] = (enc_dates[1] & 0x7F) | ((((builder._not_valid_before.month) & 0x0F) << 7) & 0xFF)
    enc_dates[1] = (enc_dates[1] & 0x83) | (((builder._not_valid_before.day & 0x1F) << 2) & 0xFF)
    enc_dates[1] = (enc_dates[1] & 0xFC) | (((builder._not_valid_before.hour & 0x1F) >> 3) & 0xFF)
    enc_dates[2] = (enc_dates[2] & 0x1F) | (((builder._not_valid_before.hour & 0x1F) << 5) & 0xFF)
    enc_dates[2] = (enc_dates[2] & 0xE0) | ((expire_years & 0x1F) & 0xFF)
    enc_dates = bytes(enc_dates)

    # SAH256 hash of the public key and encoded dates
    digest = hashes.Hash(hashes.SHA256(), backend=crypto_be)
    digest.update(pubkey)
    digest.update(enc_dates)
    raw_sn = bytearray(digest.finalize()[:size])
    raw_sn[0] = raw_sn[0] & 0x7F # Force MSB bit to 0 to ensure positive integer
    raw_sn[0] = raw_sn[0] | 0x40 # Force next bit to 1 to ensure the integer won't be trimmed in ASN.1 DER encoding
    return int.from_bytes(raw_sn, byteorder='big', signed=False)

def main():
    ############################################################################
    if len(sys.argv) != 2:
        print("You need to supply USB port as the first argument, example command: python manual_kit_provision.py /dev/tty.usbmodem1421. On Linux/MAC you can use this command to see list of available ports: ls /dev | grep 'tty.usb'")
        sys.exit()
    else:
        port = str(sys.argv[1])
    com = Serial(port, 115200)
    ############################################################################

    print('\nLoading root CA certificate')
    if not os.path.isfile(ROOT_CA_CERT_FILENAME):
        raise Exception('Failed to find root CA certificate file, ' + ROOT_CA_CERT_FILENAME + '. Have you run ca_create_root first?')
    with open(ROOT_CA_CERT_FILENAME, 'rb') as f:
        print('    Loading from ' + f.name)
        root_ca_cert = x509.load_pem_x509_certificate(f.read(), crypto_be)

    print('\nLoading signer CA key')
    if not os.path.isfile(SIGNER_CA_KEY_FILENAME):
        raise Exception('Failed to find signer CA key file, ' + SIGNER_CA_KEY_FILENAME + '. Have you run ca_create_signer_csr first?')
    with open(SIGNER_CA_KEY_FILENAME, 'rb') as f:
        print('    Loading from ' + f.name)
        signer_ca_priv_key = serialization.load_pem_private_key(
            data=f.read(),
            password=None,
            backend=crypto_be)

    print('\nLoading signer CA certificate')
    if not os.path.isfile(SIGNER_CA_CERT_FILENAME):
        raise Exception('Failed to find signer CA certificate file, ' + SIGNER_CA_CERT_FILENAME + '. Have you run ca_create_signer first?')
    with open(SIGNER_CA_CERT_FILENAME, 'rb') as f:
        print('    Loading from ' + f.name)
        signer_ca_cert = x509.load_pem_x509_certificate(f.read(), crypto_be)

    print('\nRequesting device CSR')
    ############################################################################
    com.write("genCsr:\0".encode())
    res = com.read_until(b'\x00')
    csr = res[:-1].decode("utf-8")
    print('    Done.')
    ############################################################################
    device_csr = x509.load_der_x509_csr(binascii.a2b_hex(csr), crypto_be)
    if not device_csr.is_signature_valid:
        raise Exception('Device CSR has invalid signature.')
    with open(DEVICE_CSR_FILENAME, 'wb') as f:
        print('    Saving to ' + f.name)
        f.write(device_csr.public_bytes(encoding=serialization.Encoding.PEM))

    print('\nGenerating device certificate from CSR')
    # Build certificate
    builder = x509.CertificateBuilder()
    builder = builder.issuer_name(signer_ca_cert.subject)
    builder = builder.not_valid_before(datetime.datetime.now(tz=pytz.utc).replace(minute=0,second=0)) # Device cert must have minutes and seconds set to 0
    builder = builder.not_valid_after(datetime.datetime(3000, 12, 31, 23, 59, 59)) # Should be year 9999, but this doesn't work on windows
    builder = builder.subject_name(device_csr.subject)
    builder = builder.public_key(device_csr.public_key())
    # Device certificate is generated from certificate dates and public key
    builder = builder.serial_number(device_cert_sn(16, builder))
    # Add in extensions specified by CSR
    for extension in device_csr.extensions:
        builder = builder.add_extension(extension.value, extension.critical)
    builder = builder.add_extension(
        x509.SubjectKeyIdentifier.from_public_key(builder._public_key),
        critical=False)
    issuer_ski = signer_ca_cert.extensions.get_extension_for_class(x509.SubjectKeyIdentifier)
    builder = builder.add_extension(
        x509.AuthorityKeyIdentifier.from_issuer_subject_key_identifier(issuer_ski),
        critical=False)

    # Sign certificate
    device_cert = builder.sign(
        private_key=signer_ca_priv_key,
        algorithm=hashes.SHA256(),
        backend=crypto_be)

    # Save certificate for reference
    with open(DEVICE_CERT_FILENAME, 'wb') as f:
        print('    Saving to ' + f.name)
        f.write(device_cert.public_bytes(encoding=serialization.Encoding.PEM))

    print('\nProvisioning device with Azure IoT credentials\n')
    pub_nums = root_ca_cert.public_key().public_numbers()
    pubkey =  pub_nums.x.to_bytes(32, byteorder='big', signed=False)
    pubkey += pub_nums.y.to_bytes(32, byteorder='big', signed=False)

    ############################################################################
    print("Sending Singer CA Pub Key")
    msg = "caPubKey:" + pubkey.hex() + '\0'
    com.write(msg.encode())
    res = com.read_until(b'\x00')
    print('    Done.\n')
    sleep(1)

    print("Sending Signer Certificate")
    msg = "caCert:" + signer_ca_cert.public_bytes(encoding=serialization.Encoding.DER).hex() + '\0'
    com.write(msg.encode())
    res = com.read_until(b'\x00')
    print('    Done.\n')
    sleep(1)

    print("Sending Device Certificate")
    msg = "deviceCert:" + device_cert.public_bytes(encoding=serialization.Encoding.DER).hex() + '\0'
    com.write(msg.encode())
    res = com.read_until(b'\x00')
    print('    Done.\n')
    sleep(1)

    print("Provisioning the WINC")
    com.write("transferCertificates:\0".encode())
    res = com.read_until(b'\x00')
    print('    Done.\n')
    ############################################################################
	
    subjectAttributes = str(device_csr.subject._attributes[1]).split("=")
    commonName=subjectAttributes[1].strip("<>()")

    print('Done provisioning device {}'.format(commonName))
	


try:
    main()
except Exception as e:
    print(e)
