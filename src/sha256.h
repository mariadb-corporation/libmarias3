/*
 * SHA256 hash implementation and interface functions
 * Copyright (c) 2003-2016, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SHA256_H
#define SHA256_H

#define SHA256_MAC_LEN 32

#include <stdint.h>
#include <stddef.h>
#include <string.h>

int hmac_sha256_vector(const uint8_t *key, size_t key_len, size_t num_elem,
                       const uint8_t *addr[], const size_t *len, uint8_t *mac);
int hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data,
                size_t data_len, uint8_t *mac);

int sha256(const uint8_t *addr, const size_t len, uint8_t *mac);

#endif /* SHA256_H */
