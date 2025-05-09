#!/bin/bash
# certGen.sh - Complete SSL Certificate Generator for ESP32 HTTPS

set -e  # Exit on error

# Configuration
CERTS_DIR="main/certs"
DAYS_VALID=3650
KEY_SIZE=2048
COMMON_NAME="esp32"
CA_NAME="ESP32-CA"

echo "🔐 ESP32 Certificate Installation in $CERTS_DIR"
echo "---------------------------------------------"

# Create certs directory
mkdir -p "$CERTS_DIR"
echo "📁 Created certificate directory: $CERTS_DIR"

# Generate CA
echo -e "\n🛠️  Generating Root CA..."
openssl req -x509 -newkey rsa:$KEY_SIZE \
  -keyout "$CERTS_DIR/ca.key" -out "$CERTS_DIR/ca.crt" \
  -days $DAYS_VALID -nodes -subj "/CN=$CA_NAME"

# Generate server certificate
echo -e "\n🛠️  Generating Server Certificate..."
openssl req -newkey rsa:$KEY_SIZE -nodes \
  -keyout "$CERTS_DIR/server.key" -out "$CERTS_DIR/server.csr" \
  -subj "/CN=$COMMON_NAME" \
  -addext "basicConstraints=CA:FALSE" \
  -addext "keyUsage=digitalSignature,keyEncipherment" \
  -addext "extendedKeyUsage=serverAuth"

# Sign server certificate
openssl x509 -req -in "$CERTS_DIR/server.csr" \
  -CA "$CERTS_DIR/ca.crt" -CAkey "$CERTS_DIR/ca.key" -CAcreateserial \
  -out "$CERTS_DIR/server.crt" -days $DAYS_VALID \
  -extfile <(printf "[ext]\nbasicConstraints=CA:FALSE\nkeyUsage=digitalSignature,keyEncipherment\nextendedKeyUsage=serverAuth")

# Create full chain
echo -e "\n🔗 Creating Certificate Chain..."
cat "$CERTS_DIR/server.crt" "$CERTS_DIR/ca.crt" > "$CERTS_DIR/fullchain.crt"

# Set permissions
chmod 600 "$CERTS_DIR/server.key"
chmod 644 "$CERTS_DIR/server.crt" "$CERTS_DIR/fullchain.crt" "$CERTS_DIR/ca.crt"
echo "🔒 Set secure file permissions"

# Verify certificates
echo -e "\n🔍 Verifying installation..."
echo "Certificate Chain:"
openssl x509 -in "$CERTS_DIR/fullchain.crt" -noout -text | grep -A1 "Issuer:"
echo -e "\nPrivate Key:"
openssl rsa -in "$CERTS_DIR/server.key" -check -noout

# Cleanup temporary files
rm -f "$CERTS_DIR/ca.key" "$CERTS_DIR/server.csr" "$CERTS_DIR/ca.srl"

echo -e "\n🎉 Successfully installed in $CERTS_DIR:"
ls -lh "$CERTS_DIR" | grep -E "server.key|server.crt|fullchain.crt|ca.crt"

echo -e "\n💡 Next steps:"
echo "1. Update CMakeLists.txt with either:"
echo "   EMBED_FILES \"main/certs/fullchain.crt\" \"main/certs/server.key\""
echo "   or"
echo "   EMBED_FILES \"main/certs/server.crt\" \"main/certs/server.key\""
echo "2. Add 'main/certs/server.key' to .gitignore"
echo "3. Perform a clean rebuild: idf.py fullclean build"