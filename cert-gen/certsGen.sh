#!/bin/bash
# certGen.sh - Complete SSL Certificate Generator for ESP32 HTTPS

set -e  # Exit on error

# Configuration
CERTS_DIR="main/certs"
DAYS_VALID=3650
KEY_SIZE=2048
COMMON_NAME="esp32"
CA_NAME="ESP32-CA"
CHAIN_MARKER="-----BEGIN CHAIN-----"

echo "🔐 ESP32 Certificate Installation in $CERTS_DIR"
echo "---------------------------------------------"

# Create certs directory
mkdir -p "$CERTS_DIR"
echo "📁 Created certificate directory: $CERTS_DIR"

# Generate Root CA
echo -e "\n🛠️  Generating Root CA..."
openssl req -x509 -newkey rsa:${KEY_SIZE} \
  -keyout "${CERTS_DIR}/ca.key" -out "${CERTS_DIR}/ca.crt" \
  -days ${DAYS_VALID} -nodes -subj "/CN=${CA_NAME}"

# Generate Server CSR + Key
echo -e "\n🛠️  Generating Server Certificate..."
openssl req -newkey rsa:${KEY_SIZE} -nodes \
  -keyout "${CERTS_DIR}/server.key" -out "${CERTS_DIR}/server.csr" \
  -subj "/CN=${COMMON_NAME}" \
  -addext "basicConstraints=CA:FALSE" \
  -addext "keyUsage=digitalSignature,keyEncipherment" \
  -addext "extendedKeyUsage=serverAuth"

# Sign Server Cert
openssl x509 -req -in "${CERTS_DIR}/server.csr" \
  -CA "${CERTS_DIR}/ca.crt" -CAkey "${CERTS_DIR}/ca.key" -CAcreateserial \
  -out "${CERTS_DIR}/server.crt" -days ${DAYS_VALID} \
  -extfile <(printf "[ext]\nbasicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth")

# Create fullchain.crt with explicit chain marker
echo -e "\n🔗 Creating Certificate Chain..."
{
  cat "${CERTS_DIR}/server.crt"
  printf '\n%s\n\n' "${CHAIN_MARKER}"
  cat "${CERTS_DIR}/ca.crt"
} > "${CERTS_DIR}/fullchain.crt"

# Set permissions
chmod 600 "${CERTS_DIR}/server.key"
chmod 644 "${CERTS_DIR}/server.crt" "${CERTS_DIR}/fullchain.crt" "${CERTS_DIR}/ca.crt"
echo "🔒 Set secure file permissions"

# Verify certificates
echo -e "\n🔍 Verifying Certificate Chain..."
echo "1. Verifying server certificate was signed by CA:"
if openssl verify -CAfile "${CERTS_DIR}/ca.crt" "${CERTS_DIR}/server.crt"; then
    echo "✅ Server certificate valid"
else
    echo "❌ Server certificate verification failed!"
    exit 1
fi

echo -e "\n2. Checking full chain integrity:"
if openssl verify -CAfile "${CERTS_DIR}/ca.crt" -untrusted "${CERTS_DIR}/ca.crt" "${CERTS_DIR}/server.crt"; then
    echo "✅ Full chain valid"
else
    echo "❌ Full chain verification failed!"
    exit 1
fi

echo -e "\n3. Certificate chain contents:"
echo "  • The end of the server cert:"
grep -B1 -A1 -F -- "-----BEGIN CERTIFICATE-----" "${CERTS_DIR}/fullchain.crt"
echo
echo "  • Your explicit chain marker:"
grep -C1 -F -- "${CHAIN_MARKER}" "${CERTS_DIR}/fullchain.crt"

echo -e "\n4. Chain marker verification:"
if grep -Fq -- "${CHAIN_MARKER}" "${CERTS_DIR}/fullchain.crt"; then
    echo "✅ Found chain marker in fullchain.crt"
else
    echo "❌ Chain marker not found!"
    exit 1
fi

# Cleanup temporary files
rm -f "${CERTS_DIR}/ca.key" "${CERTS_DIR}/server.csr" "${CERTS_DIR}/ca.srl"

echo -e "\n🎉 Successfully installed in $CERTS_DIR:"
ls -lh "${CERTS_DIR}" | grep -E "server.key|server.crt|fullchain.crt|ca.crt"

echo -e "\n💡 Next steps:"
echo "1. In CMakeLists.txt, embed:"
echo "     idf_component_register(EMBED_FILES \"${CERTS_DIR}/fullchain.crt\" \"${CERTS_DIR}/server.key\")"
echo "2. Add '${CERTS_DIR}/server.key' to .gitignore"
echo "3. Run a full clean & rebuild:"
echo "     idf.py fullclean build"
