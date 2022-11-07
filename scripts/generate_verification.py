import cryptography
import os
import sys
from datetime import datetime, timezone, timedelta
from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization

crypto_be = cryptography.hazmat.backends.default_backend()

regCode = sys.argv[1]

SIGNER_CA_KEY_FILENAME = "signer-ca.key"
SIGNER_CA_CERT_FILENAME = "signer-ca.cer"
SIGNER_CA_VER_CERT_FILENAME = "signer-ca-verification.cer"

def random_cert_sn(size):
    """Create a positive, non-trimmable serial number for X.509 certificates"""
    raw_sn = bytearray(os.urandom(size))
    raw_sn[0] = raw_sn[0] & 0x7F # Force MSB bit to 0 to ensure positive integer
    raw_sn[0] = raw_sn[0] | 0x40 # Force next bit to 1 to ensure the integer won't be trimmed in ASN.1 DER encoding
    return int.from_bytes(raw_sn, byteorder='big', signed=False)

# Read the signer CA key file needed to sign the verification certificate
print('\nReading signer CA key file, %s' % SIGNER_CA_KEY_FILENAME)
if not os.path.isfile(SIGNER_CA_KEY_FILENAME):
    raise Error('Failed to find signer CA key file, ' + SIGNER_CA_KEY_FILENAME + '. Have you run ca_create_signer_csr first?')
with open(SIGNER_CA_KEY_FILENAME, 'rb') as f:
    signer_ca_priv_key = serialization.load_pem_private_key(
        data=f.read(),
        password=None,
        backend=crypto_be)

# Read the signer CA certificate to be registered with Azure IoT
print('\nReading signer CA certificate file, %s' % SIGNER_CA_CERT_FILENAME)
if not os.path.isfile(SIGNER_CA_CERT_FILENAME):
    raise Error('Failed to find signer CA certificate file, ' + SIGNER_CA_CERT_FILENAME + '. Have you run ca_create_signer first?')
with open(SIGNER_CA_CERT_FILENAME, 'rb') as f:
    signer_ca_cert = x509.load_pem_x509_certificate(f.read(), crypto_be)

builder = x509.CertificateBuilder()
builder = builder.serial_number(random_cert_sn(16))
builder = builder.issuer_name(signer_ca_cert.subject)
builder = builder.not_valid_before(datetime.utcnow().replace(tzinfo=timezone.utc))
builder = builder.not_valid_after(builder._not_valid_before + timedelta(days=1))
builder = builder.subject_name(x509.Name([x509.NameAttribute(x509.oid.NameOID.COMMON_NAME, regCode)]))
builder = builder.public_key(signer_ca_cert.public_key())
signer_ca_ver_cert = builder.sign(
    private_key=signer_ca_priv_key,
    algorithm=hashes.SHA256(),
    backend=crypto_be)

with open(SIGNER_CA_VER_CERT_FILENAME, 'wb') as f:
    print('    Saved to ' + f.name)
    f.write(signer_ca_ver_cert.public_bytes(encoding=serialization.Encoding.PEM))
