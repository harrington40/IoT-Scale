// File: main/certs/server_cert.c

#include <stdint.h>
#include <stddef.h>
#include "server_cert.h"


const char* server_cert_chain_pem   = NULL;
size_t      server_cert_chain_len   = 0;
const char* server_private_key_pem  = NULL;
size_t      server_private_key_len  = 0;

void server_cert_init(void) {
    server_cert_chain_pem  = (const char*)_binary_fullchain_crt_start;
    server_cert_chain_len  = _binary_fullchain_crt_end   - _binary_fullchain_crt_start;
    server_private_key_pem = (const char*)_binary_server_key_start;
    server_private_key_len = _binary_server_key_end     - _binary_server_key_start;
}