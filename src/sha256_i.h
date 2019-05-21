/*
 * SHA-256 internal definitions
 * Copyright (c) 2003-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef SHA256_I_H
#define SHA256_I_H

#define SHA256_BLOCK_SIZE 64

#include <stdint.h>

struct sha256_state
{
  uint64_t length;
  uint32_t state[8], curlen;
  uint8_t buf[SHA256_BLOCK_SIZE];
};

void sha256_init(struct sha256_state *md);
int sha256_process(struct sha256_state *md, const unsigned char *in,
                   unsigned long inlen);
int sha256_done(struct sha256_state *md, unsigned char *out);

/**
 * sha256_vector - SHA256 hash for data vector
 * @num_elem: Number of elements in the data vector
 * @addr: Pointers to the data areas
 * @len: Lengths of the data blocks
 * @mac: Buffer for the hash
 * Returns: 0 on success, -1 on failure
 */
int sha256_vector(size_t num_elem, const uint8_t *addr[], const size_t *len,
                  uint8_t *mac);

static inline void WPA_PUT_BE64(uint8_t *a, uint64_t val)
{
  a[0] = val >> 56;
  a[1] = val >> 48;
  a[2] = val >> 40;
  a[3] = val >> 32;
  a[4] = val >> 24;
  a[5] = val >> 16;
  a[6] = val >> 8;
  a[7] = val & 0xff;
}


static inline uint32_t WPA_GET_BE32(const uint8_t *a)
{
  return ((uint32_t) a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

static inline void WPA_PUT_BE32(uint8_t *a, uint32_t val)
{
  a[0] = (val >> 24) & 0xff;
  a[1] = (val >> 16) & 0xff;
  a[2] = (val >> 8) & 0xff;
  a[3] = val & 0xff;
}


#endif /* SHA256_I_H */
