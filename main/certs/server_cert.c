// File: main/certs/server_cert.c

#include <stdint.h>
#include <stddef.h>
#include "server_cert.h"


/**
 * Must be called once (e.g. at the top of app_main) before using
 * server_cert_chain_pem/server_private_key_pem.
 */
void server_cert_init(void)
{
    server_cert_chain_pem  = (const char*)_binary_fullchain_crt_start;
    server_cert_chain_len  = (size_t)(_binary_fullchain_crt_end - _binary_fullchain_crt_start);

    server_private_key_pem = (const char*)_binary_server_key_start;
    server_private_key_len = (size_t)(_binary_server_key_end   - _binary_server_key_start);
}
