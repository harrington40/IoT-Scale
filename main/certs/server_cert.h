// File: main/certs/server_cert.h

#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Linker-generated symbols for embedded files:
extern const uint8_t _binary_fullchain_crt_start[] asm("_binary_fullchain_crt_start");
extern const uint8_t _binary_fullchain_crt_end[]   asm("_binary_fullchain_crt_end");
extern const uint8_t _binary_server_key_start[]    asm("_binary_server_key_start");
extern const uint8_t _binary_server_key_end[]      asm("_binary_server_key_end");

// Public pointers and lengths (initialized at runtime)
const char*  server_cert_chain_pem   = NULL;
size_t       server_cert_chain_len   = 0;
const char*  server_private_key_pem  = NULL;
size_t       server_private_key_len  = 0;



 
void server_cert_init(void);

#ifdef __cplusplus
}
#endif
