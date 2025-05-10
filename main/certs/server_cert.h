#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Linker-generated symbols for embedded files
extern const uint8_t _binary_fullchain_crt_start[] asm("_binary_fullchain_crt_start");
extern const uint8_t _binary_fullchain_crt_end[]   asm("_binary_fullchain_crt_end");
extern const uint8_t _binary_server_key_start[]    asm("_binary_server_key_start");
extern const uint8_t _binary_server_key_end[]      asm("_binary_server_key_end");

// Public pointers and lengths (set by server_cert_init in server_cert.c)
extern const char* server_cert_chain_pem;
extern size_t      server_cert_chain_len;
extern const char* server_private_key_pem;
extern size_t      server_private_key_len;

// Initializes the public pointers and lengths from the embedded data
void server_cert_init(void);

#ifdef __cplusplus
}
#endif