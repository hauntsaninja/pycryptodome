#include <assert.h>
#include "common.h"
#include "mont.h"
#include "ec.h"
#include "endianess.h"
#include "modexp_utils.h"

void print_x(const char *s, const uint64_t *number, const MontContext *ctx);
Workplace *new_workplace(const MontContext *ctx);
void free_workplace(Workplace *wp);

void ec_projective_to_affine(uint64_t *x3, uint64_t *y3,
                         const uint64_t *x1, const uint64_t *y1, const uint64_t *z1,
                         Workplace *tmp,
                         const MontContext *ctx);

void ec_full_double(uint64_t *x3, uint64_t *y3, uint64_t *z3,
                    const uint64_t *x1, const uint64_t *y1, const uint64_t *z1,
                    const uint64_t *b,
                    Workplace *tmp, const MontContext *ctx);
void ec_mix_add(uint64_t *x3, uint64_t *y3, uint64_t *z3,
                       const uint64_t *x1, const uint64_t *y1, const uint64_t *z1,
                       const uint64_t *x2, const uint64_t *y2,
                       const uint64_t *b,
                       Workplace *tmp,
                       const MontContext *ctx);
void ec_full_add(uint64_t *x3, uint64_t *y3, uint64_t *z3,
                        const uint64_t *x1, const uint64_t *y1, const uint64_t *z1,
                        const uint64_t *x2, const uint64_t *y2, const uint64_t *z2,
                        const uint64_t *b,
                        Workplace *tmp,
                        const MontContext *ctx);
void ec_scalar(uint64_t *x3, uint64_t *y3, uint64_t *z3,
                   const uint64_t *x1, const uint64_t *y1, const uint64_t *z1,
                   uint64_t *b,
                   const uint8_t *exp, size_t exp_size, uint64_t seed,
                   Workplace *wp1,
                   Workplace *wp2,
                   const MontContext *ctx);

int ec_scalar_g_p256(uint64_t *x3, uint64_t *y3, uint64_t *z3,
                      const uint64_t *b,
                      const uint8_t *exp, size_t exp_size,
                      uint64_t seed,
                      Workplace *wp1,
                      Workplace *wp2,
                      ProtMemory **prot_g,
                      const MontContext *ctx);
ProtMemory** ec_scramble_g_p256(const MontContext *ctx, uint64_t seed);
void free_g_p256(ProtMemory **prot_g);

static int matches(const uint64_t *x1, const uint64_t *y1, const uint64_t *z1,
                   const uint64_t *x2, const uint64_t *y2, const uint64_t *z2,
                   Workplace *wp,
                   const MontContext *ctx)
{
    uint64_t *xa, *ya, *xb, *yb;
    int result;

    mont_number(&xa, 1, ctx);
    mont_number(&ya, 1, ctx);
    mont_number(&xb, 1, ctx);
    mont_number(&yb, 1, ctx);

    ec_projective_to_affine(xa, ya, x1, y1, z1, wp, ctx);
    ec_projective_to_affine(xb, yb, x2, y2, z2, wp, ctx);

    result =  mont_is_equal(xa, xb, ctx);
    result |= mont_is_equal(ya, yb, ctx);

    free(xa);
    free(ya);
    free(xb);
    free(yb);

    return (result != 0);
}

void test_ec_projective_to_affine(void)
{
    Workplace *wp;
    MontContext *ctx;
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint64_t *x, *y, *z;
    uint8_t buffer[32];
    uint64_t zero[4] = { 0 };

    mont_context_init(&ctx, modulus, sizeof(modulus));
    wp = new_workplace(ctx);

    /** Arbitrary point on the curve with Z=10 **/
    mont_from_bytes(&x, (uint8_t*)"\xc6\x4c\x90\xad\x8d\x5c\x1d\x96\xd6\x4b\x63\x46\x4a\x8b\x57\x91\xbf\x48\xa6\xb4\xb9\xbc\xd6\xad\x79\xc6\x3a\x13\xbf\xb7\x78\x5b", 32, ctx);
    mont_from_bytes(&y, (uint8_t*)"\xe4\x98\x64\xd0\x22\x85\x75\x8a\x11\x79\x68\x2e\x06\x92\x3d\xf7\x62\xa8\x85\xea\xda\xe6\xd9\xb0\x5a\x4f\x0c\x43\x1d\x51\x77\xe4", 32, ctx);
    mont_from_bytes(&z, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    ec_projective_to_affine(x, y, x, y, z, wp, ctx);
    mont_to_bytes(buffer, 32, x, ctx);
    assert(0 == memcmp(buffer, (uint8_t*)"\xfa\x3a\xdb\x43\xa7\xbc\x69\x5c\xc8\xa1\x23\x87\x07\x74\x55\x8e\x93\x20\xdd\x79\x5f\x5f\xaf\x11\x58\xfa\x39\x01\xf9\x92\x58\xd5", 32));
    mont_to_bytes(buffer, 32, y, ctx);
    assert(0 == memcmp(buffer, (uint8_t*)"\xe3\xa8\xd6\xe0\xd0\x40\x8b\xc1\xce\x8c\x24\x04\x9a\x41\xd2\xff\x23\x77\x40\x98\x49\x17\x15\xc4\xd5\xd4\xb4\x6d\x1c\x88\x25\x96", 32));

    /** Point-at-infinity **/
    memset(x, 0xFF, 32);
    memset(y, 0xFF, 32);
    memset(z, 0, 32);
    ec_projective_to_affine(x, y, x, y, z, wp, ctx);
    assert(0 == memcmp(x, zero, 32));
    assert(0 == memcmp(y, zero, 32));

    free(x);
    free(y);
    free(z);

    free_workplace(wp);
    mont_context_free(ctx);
}

void test_ec_full_double(void)
{
    Workplace *wp;
    MontContext *ctx;
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint64_t *x, *y, *z;
    uint64_t *b;
    uint8_t buffer[32];
    uint64_t zero[4] = { 0 };

    mont_context_init(&ctx, modulus, sizeof(modulus));
    wp = new_workplace(ctx);
    mont_from_bytes(&b, (uint8_t*)"\x5a\xc6\x35\xd8\xaa\x3a\x93\xe7\xb3\xeb\xbd\x55\x76\x98\x86\xbc\x65\x1d\x06\xb0\xcc\x53\xb0\xf6\x3b\xce\x3c\x3e\x27\xd2\x60\x4b", 32, ctx);

    /** Arbitrary point on the curve with Z=10 **/
    mont_from_bytes(&x, (uint8_t*)"\xc6\x4c\x90\xad\x8d\x5c\x1d\x96\xd6\x4b\x63\x46\x4a\x8b\x57\x91\xbf\x48\xa6\xb4\xb9\xbc\xd6\xad\x79\xc6\x3a\x13\xbf\xb7\x78\x5b", 32, ctx);
    mont_from_bytes(&y, (uint8_t*)"\xe4\x98\x64\xd0\x22\x85\x75\x8a\x11\x79\x68\x2e\x06\x92\x3d\xf7\x62\xa8\x85\xea\xda\xe6\xd9\xb0\x5a\x4f\x0c\x43\x1d\x51\x77\xe4", 32, ctx);
    mont_from_bytes(&z, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    ec_full_double(x, y, z, x, y, z, b, wp, ctx);

    mont_to_bytes(buffer, 32, x, ctx);
    assert(0 == memcmp(buffer, "\x9e\x0e\xcb\x70\xd6\x15\x88\x5e\x6a\xce\x5a\x03\x41\x89\xd5\xe5\xf8\xb1\x6f\x38\xe5\xc0\x1e\x59\xf5\xcc\xe6\xdf\xb4\xf9\xdd\x02", 32));
    mont_to_bytes(buffer, 32, y, ctx);
    assert(0 == memcmp(buffer, "\xda\x30\xad\x21\x7a\x5d\xe2\x3a\xd6\x86\x12\xd2\x61\xa0\x7b\x51\xff\x05\x3c\x73\xa6\x63\x88\x4b\xa4\xe6\xb6\x84\x71\x9a\xe0\xb4", 32));
    mont_to_bytes(buffer, 32, z, ctx);
    assert(0 == memcmp(buffer, "\x62\x60\x97\xCF\xE5\x64\xFC\xD1\x02\x41\xD7\xD1\x63\xBE\xF2\x41\x3D\xA9\xA8\xD6\x60\x5B\x7B\xB5\x7C\x4E\x4A\x21\x69\xA5\xFA\xC2", 32));

    /** Point-at-infinity **/
    mont_set(x, 0, ctx);
    mont_set(y, 1, ctx);
    mont_set(z, 0, ctx);
    ec_full_double(x, y, z, x, y, z, b, wp, ctx);
    assert(0 == memcmp(z, zero, 32));

    /* Points with Y=0; for P-256:
       X = 0x512aecbfc85c47596a7fb7b1285159e35f22b92edfb04634ea63c40cb6134872
       X = 0xaed5133f37a3b8a79580484ed7aea61ca0dd46d2204fb9cb159c3bf349ecb790
    */
    free(x);
    free(z);
    mont_from_bytes(&x, (uint8_t*)"\x2b\xad\x3f\x80\xd3\x9a\xc9\x7b\x28\xfd\x2c\xeb\x93\x2d\x82\xe1\xb7\x5b\x3b\xd1\xbc\xe2\xbe\x11\x27\xe5\xa8\x7f\x1c\xc0\xd4\x77", 32, ctx);
    mont_set(y, 0, ctx);
    mont_from_bytes(&z, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    ec_full_double(x, y, z, x, y, z, b, wp, ctx);
    assert(0 == memcmp(z, zero, 32));

    free(x);
    free(y);
    free(z);

    free(b);
    free_workplace(wp);
    mont_context_free(ctx);
}

void test_ec_mix_add(void)
{
    Workplace *wp;
    MontContext *ctx;
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint64_t *x1, *y1, *z1;
    uint64_t *x2, *y2;
    uint64_t *b;
    uint8_t buffer[32];

    mont_context_init(&ctx, modulus, sizeof(modulus));
    wp = new_workplace(ctx);
    mont_from_bytes(&b, (uint8_t*)"\x5a\xc6\x35\xd8\xaa\x3a\x93\xe7\xb3\xeb\xbd\x55\x76\x98\x86\xbc\x65\x1d\x06\xb0\xcc\x53\xb0\xf6\x3b\xce\x3c\x3e\x27\xd2\x60\x4b", 32, ctx);

    /* Arbitrary points */
    mont_from_bytes(&x1, (uint8_t*)"\xc6\x4c\x90\xad\x8d\x5c\x1d\x96\xd6\x4b\x63\x46\x4a\x8b\x57\x91\xbf\x48\xa6\xb4\xb9\xbc\xd6\xad\x79\xc6\x3a\x13\xbf\xb7\x78\x5b", 32, ctx);
    mont_from_bytes(&y1, (uint8_t*)"\xe4\x98\x64\xd0\x22\x85\x75\x8a\x11\x79\x68\x2e\x06\x92\x3d\xf7\x62\xa8\x85\xea\xda\xe6\xd9\xb0\x5a\x4f\x0c\x43\x1d\x51\x77\xe4", 32, ctx);
    mont_from_bytes(&z1, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    mont_from_bytes(&x2, (uint8_t*)"\xf2\x49\x10\x4d\x0e\x6f\x8f\x29\xe6\x01\x62\x77\x78\x0c\xda\x84\xdc\x84\xb8\x3b\xc3\xd8\x99\xdf\xb7\x36\xca\x08\x31\xfb\xe8\xcf", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\xb5\x7e\x12\xfc\xdb\x03\x1f\x59\xca\xb8\x1b\x1c\x6b\x1e\x1c\x07\xe4\x51\x2e\x52\xce\x83\x2f\x1a\x0c\xed\xef\xff\x8b\x43\x40\xe9", 32, ctx);
    ec_mix_add(x1, y1, z1, x1, y1, z1, x2, y2, b, wp, ctx);

    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x13\x6c\x0e\x59\xbe\x5f\xb3\x6f\x98\xce\x1e\xa8\xc3\x14\xf2\xef\x9f\x9e\x53\x99\x8f\xd7\x14\x3a\x98\x86\xeb\x74\xc8\x5a\xf1\xaf", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\xcd\x8f\xd0\xf2\xb2\x1f\xcc\x70\xae\x5a\x7f\xa8\x89\xac\xb8\xa2\x01\xc7\x70\xf9\xa3\xaf\x47\x4a\xc1\xb9\xf1\xc5\x62\xf7\x73\x9a", 32));
    mont_to_bytes(buffer, 32, z1, ctx);
    assert(0 == memcmp(buffer, "\x7f\x3f\xac\x7e\x49\x3e\x61\x4b\x52\xd8\x49\x31\x8b\x57\xa7\xec\x89\x50\x27\xdb\x75\xbe\xa6\x61\x3c\x54\x42\x89\xb3\x9f\x31\x46", 32));

    /* Affine input point is point-at-infinity */
    memset(x2, 0, 32);
    memset(y2, 0, 32);
    ec_mix_add(x1, y1, z1, x1, y1, z1, x2, y2, b, wp, ctx);

    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x13\x6c\x0e\x59\xbe\x5f\xb3\x6f\x98\xce\x1e\xa8\xc3\x14\xf2\xef\x9f\x9e\x53\x99\x8f\xd7\x14\x3a\x98\x86\xeb\x74\xc8\x5a\xf1\xaf", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\xcd\x8f\xd0\xf2\xb2\x1f\xcc\x70\xae\x5a\x7f\xa8\x89\xac\xb8\xa2\x01\xc7\x70\xf9\xa3\xaf\x47\x4a\xc1\xb9\xf1\xc5\x62\xf7\x73\x9a", 32));
    mont_to_bytes(buffer, 32, z1, ctx);
    assert(0 == memcmp(buffer, "\x7f\x3f\xac\x7e\x49\x3e\x61\x4b\x52\xd8\x49\x31\x8b\x57\xa7\xec\x89\x50\x27\xdb\x75\xbe\xa6\x61\x3c\x54\x42\x89\xb3\x9f\x31\x46", 32));

    /* Projective input point is point-at-infinity */
    mont_set(x1, 0, ctx);
    mont_set(y1, 1, ctx);
    mont_set(z1, 0, ctx);

    mont_from_bytes(&x2, (uint8_t*)"\xf2\x49\x10\x4d\x0e\x6f\x8f\x29\xe6\x01\x62\x77\x78\x0c\xda\x84\xdc\x84\xb8\x3b\xc3\xd8\x99\xdf\xb7\x36\xca\x08\x31\xfb\xe8\xcf", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\xb5\x7e\x12\xfc\xdb\x03\x1f\x59\xca\xb8\x1b\x1c\x6b\x1e\x1c\x07\xe4\x51\x2e\x52\xce\x83\x2f\x1a\x0c\xed\xef\xff\x8b\x43\x40\xe9", 32, ctx);

    ec_mix_add(x1, y1, z1, x1, y1, z1, x2, y2, b, wp, ctx);

    ec_projective_to_affine(x1, y1, x1, y1, z1, wp, ctx);
    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, (uint8_t*)"\xf2\x49\x10\x4d\x0e\x6f\x8f\x29\xe6\x01\x62\x77\x78\x0c\xda\x84\xdc\x84\xb8\x3b\xc3\xd8\x99\xdf\xb7\x36\xca\x08\x31\xfb\xe8\xcf", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, (uint8_t*)"\xb5\x7e\x12\xfc\xdb\x03\x1f\x59\xca\xb8\x1b\x1c\x6b\x1e\x1c\x07\xe4\x51\x2e\x52\xce\x83\x2f\x1a\x0c\xed\xef\xff\x8b\x43\x40\xe9", 32));

    /* Affine and projective are actually the same point (doubling) */
    mont_from_bytes(&x1, (uint8_t*)"\xc6\x4c\x90\xad\x8d\x5c\x1d\x96\xd6\x4b\x63\x46\x4a\x8b\x57\x91\xbf\x48\xa6\xb4\xb9\xbc\xd6\xad\x79\xc6\x3a\x13\xbf\xb7\x78\x5b", 32, ctx);
    mont_from_bytes(&y1, (uint8_t*)"\xe4\x98\x64\xd0\x22\x85\x75\x8a\x11\x79\x68\x2e\x06\x92\x3d\xf7\x62\xa8\x85\xea\xda\xe6\xd9\xb0\x5a\x4f\x0c\x43\x1d\x51\x77\xe4", 32, ctx);
    mont_from_bytes(&z1, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);

    mont_from_bytes(&x2, (uint8_t*)"\xfa\x3a\xdb\x43\xa7\xbc\x69\x5c\xc8\xa1\x23\x87\x07\x74\x55\x8e\x93\x20\xdd\x79\x5f\x5f\xaf\x11\x58\xfa\x39\x01\xf9\x92\x58\xd5", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\xe3\xa8\xd6\xe0\xd0\x40\x8b\xc1\xce\x8c\x24\x04\x9a\x41\xd2\xff\x23\x77\x40\x98\x49\x17\x15\xc4\xd5\xd4\xb4\x6d\x1c\x88\x25\x96", 32, ctx);

    ec_mix_add(x1, y1, z1, x1, y1, z1, x2, y2, b, wp, ctx);

    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x96\x0f\x82\x08\x3a\x75\xf9\xaf\x9a\xab\x06\x05\x27\x0e\x2d\xa8\xb3\x20\x7e\x8d\xf2\xf0\x00\x4d\xb3\x19\x16\xc9\xea\xc5\x0f\x02", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\x20\xe6\xe3\x02\xc6\x57\xfa\x95\x30\x39\xa9\x25\xf1\x9d\xc3\xcb\x0f\x59\xa7\x01\x46\xc8\xac\xe2\x09\x54\x3a\x25\x2a\x18\x96\xba", 32));
    mont_to_bytes(buffer, 32, z1, ctx);
    assert(0 == memcmp(buffer, "\xb4\x2f\x0b\xc1\x61\x03\x91\xe4\x11\xf1\x4c\x65\xef\x13\xd4\x57\xb1\x41\xb2\x54\xc3\x86\x08\xea\xc6\x5c\xf1\x61\x9d\x37\x6b\x77", 32));

    free(x1);
    free(y1);
    free(z1);
    free(x2);
    free(y2);

    free(b);
    free_workplace(wp);
    mont_context_free(ctx);
}

void test_ec_full_add(void)
{
    Workplace *wp;
    MontContext *ctx;
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint64_t *x1, *y1, *z1;
    uint64_t *x2, *y2, *z2;
    uint64_t *x3, *y3, *z3;
    uint64_t *b;
    uint8_t buffer[32];

    mont_context_init(&ctx, modulus, sizeof(modulus));
    wp = new_workplace(ctx);
    mont_from_bytes(&b, (uint8_t*)"\x5a\xc6\x35\xd8\xaa\x3a\x93\xe7\xb3\xeb\xbd\x55\x76\x98\x86\xbc\x65\x1d\x06\xb0\xcc\x53\xb0\xf6\x3b\xce\x3c\x3e\x27\xd2\x60\x4b", 32, ctx);

    /* Arbitrary points */
    mont_from_bytes(&x1, (uint8_t*)"\xc6\x4c\x90\xad\x8d\x5c\x1d\x96\xd6\x4b\x63\x46\x4a\x8b\x57\x91\xbf\x48\xa6\xb4\xb9\xbc\xd6\xad\x79\xc6\x3a\x13\xbf\xb7\x78\x5b", 32, ctx);
    mont_from_bytes(&y1, (uint8_t*)"\xe4\x98\x64\xd0\x22\x85\x75\x8a\x11\x79\x68\x2e\x06\x92\x3d\xf7\x62\xa8\x85\xea\xda\xe6\xd9\xb0\x5a\x4f\x0c\x43\x1d\x51\x77\xe4", 32, ctx);
    mont_from_bytes(&z1, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    mont_from_bytes(&x2, (uint8_t*)"\x15\xa0\x46\x37\xa6\x49\xfc\x67\x7a\xb4\xd0\x33\x25\x56\x7d\x14\xb9\xe8\x3a\xbf\x1a\xd1\xe4\x4e\xfa\x1c\x41\xc8\x2f\xb6\x76\x7e", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\x4b\x3f\xda\x5a\xa0\xaa\xd1\x9f\x4c\xb6\x60\xa8\x24\x50\xf8\xa3\x7a\x8b\x43\x9e\xf0\x93\x35\x53\xe6\x0f\x54\xa6\xae\xd6\x4a\x83", 32, ctx);
    mont_from_bytes(&z2, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    ec_full_add(x1, y1, z1, x1, y1, z1, x2, y2, z2, b, wp, ctx);

    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x0a\x49\xc8\x32\x3a\x4f\x13\x47\x40\x5a\x25\x43\xd4\xc1\xc6\xc8\xf7\x74\x51\xc5\x83\x5c\x82\x20\x9b\x39\x9c\x23\xee\xf6\x29\x2a", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\xaa\xb2\xaa\x93\x13\x26\xc2\xfe\x0f\x44\xd3\x2d\xfe\x19\x57\x6e\x6b\xa9\x9a\xc9\x50\xff\x6f\x73\xb5\x8d\xa4\x39\x4c\xf9\x29\xc2", 32));
    mont_to_bytes(buffer, 32, z1, ctx);
    assert(0 == memcmp(buffer, "\x7e\x70\xd6\x49\x38\x7c\xde\xe8\x7d\xbc\xe8\x58\x88\xce\xe4\xd9\x33\x50\x9b\xff\x02\xc6\x4f\x0f\x83\x30\xde\x9c\xf6\x38\x4e\xd4", 32));

    /* Same point (doubling) */
    free(x1); free(y1); free(z1); free(x2); free(y2); free(z2);
    mont_from_bytes(&x1, (uint8_t*)"\xc6\x4c\x90\xad\x8d\x5c\x1d\x96\xd6\x4b\x63\x46\x4a\x8b\x57\x91\xbf\x48\xa6\xb4\xb9\xbc\xd6\xad\x79\xc6\x3a\x13\xbf\xb7\x78\x5b", 32, ctx);
    mont_from_bytes(&y1, (uint8_t*)"\xe4\x98\x64\xd0\x22\x85\x75\x8a\x11\x79\x68\x2e\x06\x92\x3d\xf7\x62\xa8\x85\xea\xda\xe6\xd9\xb0\x5a\x4f\x0c\x43\x1d\x51\x77\xe4", 32, ctx);
    mont_from_bytes(&z1, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    mont_from_bytes(&x2, (uint8_t*)"\xfa\x3a\xdb\x43\xa7\xbc\x69\x5c\xc8\xa1\x23\x87\x07\x74\x55\x8e\x93\x20\xdd\x79\x5f\x5f\xaf\x11\x58\xfa\x39\x01\xf9\x92\x58\xd5", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\xe3\xa8\xd6\xe0\xd0\x40\x8b\xc1\xce\x8c\x24\x04\x9a\x41\xd2\xff\x23\x77\x40\x98\x49\x17\x15\xc4\xd5\xd4\xb4\x6d\x1c\x88\x25\x96", 32, ctx);
    mont_from_bytes(&z2, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01", 32, ctx);

    ec_full_add(x1, y1, z1, x1, y1, z1, x2, y2, z2, b, wp, ctx);

    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x96\x0f\x82\x08\x3a\x75\xf9\xaf\x9a\xab\x06\x05\x27\x0e\x2d\xa8\xb3\x20\x7e\x8d\xf2\xf0\x00\x4d\xb3\x19\x16\xc9\xea\xc5\x0f\x02", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\x20\xe6\xe3\x02\xc6\x57\xfa\x95\x30\x39\xa9\x25\xf1\x9d\xc3\xcb\x0f\x59\xa7\x01\x46\xc8\xac\xe2\x09\x54\x3a\x25\x2a\x18\x96\xba", 32));
    mont_to_bytes(buffer, 32, z1, ctx);
    assert(0 == memcmp(buffer, "\xb4\x2f\x0b\xc1\x61\x03\x91\xe4\x11\xf1\x4c\x65\xef\x13\xd4\x57\xb1\x41\xb2\x54\xc3\x86\x08\xea\xc6\x5c\xf1\x61\x9d\x37\x6b\x77", 32));

    /* Opposite points */
    mont_set(y2, 0, ctx);
    mont_sub(y2, y2, y1, wp->scratch, ctx);
    ec_full_add(x1, y1, z1, x1, y1, z1, x1, y2, z1, b, wp, ctx);
    assert(mont_is_zero(z1, ctx));

    /* Point at infinity (first term) */
    free(x1); free(y1); free(z1);
    mont_from_bytes(&x1, (uint8_t*)"\xf3\x91\x4a\x3a\xf2\x1b\x11\x44\x58\x3e\xf2\xf8\x54\x01\x4b\x72\xfa\x94\x05\x8d\xf9\x7c\x32\x4f\x1a\xef\x49\x37\x3c\xe8\x5b\xef", 32, ctx);
    mont_from_bytes(&y1, (uint8_t*)"\x23\xaa\x65\x85\x4c\xc5\xbc\x53\x0d\x4f\xe7\x3e\xd9\x58\x95\x67\xb2\xea\x79\x1a\x7c\x9b\xe5\xf6\x78\x8c\xd5\xbe\xd8\x55\x0d\xe7", 32, ctx);
    mont_from_bytes(&z1, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    mont_set(x2, 0, ctx);
    mont_set(y2, 1, ctx);
    mont_set(z2, 0, ctx);

    ec_full_add(x2, y2, z2, x2, y2, z2, x1, y1, z1, b, wp, ctx);

    ec_projective_to_affine(x1, y1, x1, y1, z1, wp, ctx);
    ec_projective_to_affine(x2, y2, x2, y2, z2, wp, ctx);

    assert(mont_is_equal(x1, x2, ctx));
    assert(mont_is_equal(y1, y2, ctx));

    free(x1); free(y1); free(z1);
    mont_from_bytes(&x1, (uint8_t*)"\xf3\x91\x4a\x3a\xf2\x1b\x11\x44\x58\x3e\xf2\xf8\x54\x01\x4b\x72\xfa\x94\x05\x8d\xf9\x7c\x32\x4f\x1a\xef\x49\x37\x3c\xe8\x5b\xef", 32, ctx);
    mont_from_bytes(&y1, (uint8_t*)"\x23\xaa\x65\x85\x4c\xc5\xbc\x53\x0d\x4f\xe7\x3e\xd9\x58\x95\x67\xb2\xea\x79\x1a\x7c\x9b\xe5\xf6\x78\x8c\xd5\xbe\xd8\x55\x0d\xe7", 32, ctx);
    mont_from_bytes(&z1, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);
    mont_set(x2, 0, ctx);
    mont_set(y2, 1, ctx);
    mont_set(z2, 0, ctx);
    mont_number(&x3, 1, ctx);
    mont_number(&y3, 1, ctx);
    mont_number(&z3, 1, ctx);

    ec_full_add(x3, y3, z3, x1, y1, z1, x2, y2, z2, b, wp, ctx);

    ec_projective_to_affine(x1, y1, x1, y1, z1, wp, ctx);
    ec_projective_to_affine(x3, y3, x3, y3, z3, wp, ctx);

    assert(mont_is_equal(x1, x3, ctx));
    assert(mont_is_equal(y1, y3, ctx));

    free(x1);
    free(y1);
    free(z1);
    free(x2);
    free(y2);
    free(z2);
    free(x3);
    free(y3);
    free(z3);

    free(b);
    free_workplace(wp);
    mont_context_free(ctx);
}

void test_ec_scalar(void)
{
    Workplace *wp1, *wp2;
    MontContext *ctx;
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint64_t *x1, *y1, *z1;
    uint64_t *x2, *y2, *z2;
    uint64_t *b;
    uint8_t buffer[32];

    mont_context_init(&ctx, modulus, sizeof(modulus));
    wp1 = new_workplace(ctx);
    wp2 = new_workplace(ctx);
    mont_from_bytes(&b, (uint8_t*)"\x5a\xc6\x35\xd8\xaa\x3a\x93\xe7\xb3\xeb\xbd\x55\x76\x98\x86\xbc\x65\x1d\x06\xb0\xcc\x53\xb0\xf6\x3b\xce\x3c\x3e\x27\xd2\x60\x4b", 32, ctx);

    /* 1*G */
    mont_number(&x1, 1, ctx);
    mont_number(&y1, 1, ctx);
    mont_number(&z1, 1, ctx);
    mont_from_bytes(&x2, (uint8_t*)"\x2e\xee\x33\x80\xcb\xba\x96\xcb\xb7\x61\x04\xf5\xe4\x6a\x89\x78\xa6\x22\xe7\x07\xcb\x30\x04\x49\x8e\x4c\x3c\xba\x75\xf7\x99\xe0", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\x1e\xe0\x9c\xe0\xed\x08\xfc\x10\x95\x0f\x30\xe8\xd8\x9c\x2c\xdd\xb6\x0e\x01\x67\x2f\xed\xb4\x13\xf5\x1e\x84\x12\x2d\x79\x33\x95", 32, ctx);
    mont_from_bytes(&z2, (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a", 32, ctx);

    ec_scalar(x1, y1, z1, x2, y2, z2, b, (uint8_t*)"\x01", 1, 0x4545, wp1, wp2, ctx);

    assert(matches(x1, y1, z1, x2, y2, z2, wp1, ctx));

    ec_scalar(x1, y1, z1, x2, y2, z2, b, (uint8_t*)"\x00\x01", 2, 0x4545, wp1, wp2, ctx);

    assert(matches(x1, y1, z1, x2, y2, z2, wp1, ctx));

    /* (order+1)*G */
    ec_scalar(x1, y1, z1, x2, y2, z2, b, (uint8_t*)"\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xbc\xe6\xfa\xad\xa7\x17\x9e\x84\xf3\xb9\xca\xc2\xfc\x63\x25\x52", 32, 0x4545, wp1, wp2, ctx);
    ec_projective_to_affine(x1, y1, x1, y1, z1, wp1, ctx);
    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x6b\x17\xd1\xf2\xe1\x2c\x42\x47\xf8\xbc\xe6\xe5\x63\xa4\x40\xf2\x77\x03\x7d\x81\x2d\xeb\x33\xa0\xf4\xa1\x39\x45\xd8\x98\xc2\x96", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\x4f\xe3\x42\xe2\xfe\x1a\x7f\x9b\x8e\xe7\xeb\x4a\x7c\x0f\x9e\x16\x2b\xce\x33\x57\x6b\x31\x5e\xce\xcb\xb6\x40\x68\x37\xbf\x51\xf5", 32));

    /* 0*G */
    ec_scalar(x1, y1, z1, x2, y2, z2, b, (uint8_t*)"\x00", 1, 0x4545, wp1, wp2, ctx);
    assert(mont_is_zero(z1, ctx));

    /* order*G */
    ec_scalar(x1, y1, z1, x2, y2, z2, b, (uint8_t*)"\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xbc\xe6\xfa\xad\xa7\x17\x9e\x84\xf3\xb9\xca\xc2\xfc\x63\x25\x51", 32, 0x4545, wp1, wp2, ctx);
    assert(mont_is_zero(z1, ctx));

    /* 255*O */
    ec_scalar(x1, y1, z1, x1, y1, z1, b, (uint8_t*)"\x00\xFF", 2, 0x4545, wp1, wp2, ctx);
    assert(mont_is_zero(z1, ctx));

    /* arb */
    free(x2);
    free(y2);
    mont_from_bytes(&x2, (uint8_t*)"\xde\x24\x44\xbe\xbc\x8d\x36\xe6\x82\xed\xd2\x7e\x0f\x27\x15\x08\x61\x75\x19\xb3\x22\x1a\x8f\xa0\xb7\x7c\xab\x39\x89\xda\x97\xc9", 32, ctx);
    mont_from_bytes(&y2, (uint8_t*)"\xc0\x93\xae\x7f\xf3\x6e\x53\x80\xfc\x01\xa5\xaa\xd1\xe6\x66\x59\x70\x2d\xe8\x0f\x53\xce\xc5\x76\xb6\x35\x0b\x24\x30\x42\xa2\x56", 32, ctx);
    mont_set(z2, 1, ctx);
    ec_scalar(x1, y1, z1, x2, y2, z2, b, (uint8_t*)"\xc5\x1e\x47\x53\xaf\xde\xc1\xe6\xb6\xc6\xa5\xb9\x92\xf4\x3f\x8d\xd0\xc7\xa8\x93\x30\x72\x70\x8b\x65\x22\x46\x8b\x2f\xfb\x06\xfd", 32, 0x4545, wp1, wp2, ctx);
    ec_projective_to_affine(x1, y1, x1, y1, z1, wp1, ctx);
    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x51\xd0\x8d\x5f\x2d\x42\x78\x88\x29\x46\xd8\x8d\x83\xc9\x7d\x11\xe6\x2b\xec\xc3\xcf\xc1\x8b\xed\xac\xc8\x9b\xa3\x4e\xec\xa0\x3f", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\x75\xee\x68\xeb\x8b\xf6\x26\xaa\x5b\x67\x3a\xb5\x1f\x6e\x74\x4e\x06\xf8\xfc\xf8\xa6\xc0\xcf\x30\x35\xbe\xca\x95\x6a\x7b\x41\xd5", 32));

    free(x1);
    free(y1);
    free(z1);
    free(x2);
    free(y2);
    free(z2);

    free(b);
    free_workplace(wp1);
    free_workplace(wp2);
    mont_context_free(ctx);
}

void test_ec_scalar_g_p256(void)
{
    Workplace *wp1, *wp2;
    MontContext *ctx;
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    uint64_t *b;
    int res;

    uint64_t *x1, *y1, *z1;
    uint64_t *xw, *yw;
    uint64_t *Gx_mont, *Gy_mont;
    uint8_t buffer[32];
    ProtMemory **prot_g;

    mont_context_init(&ctx, modulus, sizeof(modulus));
    wp1 = new_workplace(ctx);
    wp2 = new_workplace(ctx);
    mont_from_bytes(&b, (uint8_t*)"\x5a\xc6\x35\xd8\xaa\x3a\x93\xe7\xb3\xeb\xbd\x55\x76\x98\x86\xbc\x65\x1d\x06\xb0\xcc\x53\xb0\xf6\x3b\xce\x3c\x3e\x27\xd2\x60\x4b", 32, ctx);
    prot_g = ec_scramble_g_p256(ctx, 0x1010);

    mont_from_bytes(&Gx_mont, Gx, sizeof Gx, ctx);
    mont_from_bytes(&Gy_mont, Gy, sizeof Gy, ctx);
    mont_number(&xw, 1, ctx);
    mont_number(&yw, 1, ctx);

    /* 1*G */
    mont_number(&x1, 1, ctx);
    mont_number(&y1, 1, ctx);
    mont_number(&z1, 1, ctx);
    res = ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\x01", 1, 0x4545, wp1, wp2, prot_g, ctx);
    assert(res == 0);
    ec_projective_to_affine(xw, yw, x1, y1, z1, wp1, ctx);
    assert(mont_is_equal(xw, Gx_mont, ctx));
    assert(mont_is_equal(yw, Gy_mont, ctx));

    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\x00\x01", 2, 0x4545, wp1, wp2, prot_g, ctx);
    ec_projective_to_affine(xw, yw, x1, y1, z1, wp1, ctx);
    assert(mont_is_equal(xw, Gx_mont, ctx));
    assert(mont_is_equal(yw, Gy_mont, ctx));

    /* 0*G */
    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\x00", 1, 0x4545, wp1, wp2, prot_g, ctx);
    assert(mont_is_zero(z1, ctx));

    /* 31*G */
    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\x1F", 1, 0x4545, wp1, wp2, prot_g, ctx);
    ec_projective_to_affine(xw, yw, x1, y1, z1, wp1, ctx);
    mont_to_bytes(buffer, 32, xw, ctx);
    assert(0 == memcmp(buffer, "\x30\x1d\x9e\x50\x2d\xc7\xe0\x5d\xa8\x5d\xa0\x26\xa7\xae\x9a\xa0\xfa\xc9\xdb\x7d\x52\xa9\x5b\x3e\x3e\x3f\x9a\xa0\xa1\xb4\x5b\x8b", 32));
    mont_to_bytes(buffer, 32, yw, ctx);
    assert(0 == memcmp(buffer, "\x65\x51\xb6\xf6\xb3\x06\x12\x23\xe0\xd2\x3c\x02\x6b\x01\x7d\x72\x29\x8d\x9a\xe4\x68\x87\xca\x61\xd5\x8d\xb6\xae\xa1\x7e\xe2\x67", 32));

    /* 32*G */
    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\x20", 1, 0x4545, wp1, wp2, prot_g, ctx);
    ec_projective_to_affine(xw, yw, x1, y1, z1, wp1, ctx);
    mont_to_bytes(buffer, 32, xw, ctx);
    assert(0 == memcmp(buffer, "\x23\x77\xc7\xd6\x90\xa2\x42\xca\x6c\x45\x07\x4e\x8e\xa5\xbe\xef\xaa\x55\x7f\xd5\xb6\x83\x71\xd9\xd1\x47\x5b\xd5\x2a\x7e\xd0\xe1", 32));
    mont_to_bytes(buffer, 32, yw, ctx);
    assert(0 == memcmp(buffer, "\x47\xa1\x3f\xb9\x84\x13\xa4\x39\x3f\x8d\x90\xe9\xbf\x90\x1b\x7e\x66\x58\xa6\xcd\xec\xf4\x67\x16\xe7\xc0\x67\xb1\xdd\xb8\xd2\xb2", 32));

    /* (order+1)*G */
    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xbc\xe6\xfa\xad\xa7\x17\x9e\x84\xf3\xb9\xca\xc2\xfc\x63\x25\x52", 32, 0x4545, wp1, wp2, prot_g, ctx);
    ec_projective_to_affine(x1, y1, x1, y1, z1, wp1, ctx);
    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\x6b\x17\xd1\xf2\xe1\x2c\x42\x47\xf8\xbc\xe6\xe5\x63\xa4\x40\xf2\x77\x03\x7d\x81\x2d\xeb\x33\xa0\xf4\xa1\x39\x45\xd8\x98\xc2\x96", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\x4f\xe3\x42\xe2\xfe\x1a\x7f\x9b\x8e\xe7\xeb\x4a\x7c\x0f\x9e\x16\x2b\xce\x33\x57\x6b\x31\x5e\xce\xcb\xb6\x40\x68\x37\xbf\x51\xf5", 32));

    /* order*G */
    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xbc\xe6\xfa\xad\xa7\x17\x9e\x84\xf3\xb9\xca\xc2\xfc\x63\x25\x51", 32, 0x4545, wp1, wp2, prot_g, ctx);
    assert(mont_is_zero(z1, ctx));

    /* arbirtrary */
    ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\x73\x87\x34\x34\x3F\xF8\x93\x87", 8, 0x6776, wp1, wp2, prot_g, ctx);
    ec_projective_to_affine(x1, y1, x1, y1, z1, wp1, ctx);
    mont_to_bytes(buffer, 32, x1, ctx);
    assert(0 == memcmp(buffer, "\xfc\x85\x6a\x26\x35\x51\x2a\x83\x44\x35\x55\x97\xbd\xbf\xa9\x3d\x33\x70\x2a\x48\xb0\x9d\x02\xbd\x1d\xc4\xfd\x4b\x5a\x4c\x6c\x09", 32));
    mont_to_bytes(buffer, 32, y1, ctx);
    assert(0 == memcmp(buffer, "\xcf\x0d\xc7\x68\x18\x61\xa0\xb7\x29\x22\xa9\xce\x17\xf1\x58\x22\x31\x1a\xab\x2a\x14\xc4\xbd\xb0\xc4\x32\xea\xfe\x93\x9a\x4a\x47", 32));

    /* exponent is too long */
    res = ec_scalar_g_p256(x1, y1, z1, b, (uint8_t*)"\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xbc\xe6\xfa\xad\xa7\x17\x9e\x84\xf3\xb9\xca\xc2\xfc\x63\x25\x52\xFF", 33, 0x4545, wp1, wp2, prot_g, ctx);
    assert(res == ERR_VALUE);

    free_g_p256(prot_g);
    free(b);
    free(x1);
    free(y1);
    free(z1);
    free(xw);
    free(yw);
    free(Gx_mont);
    free(Gy_mont);
    free_workplace(wp1);
    free_workplace(wp2);
    mont_context_free(ctx);

}


void test_ec_ws_new_point(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp;
    int res;
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gx_wrong[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x97};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    const uint8_t b[32] = {0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7,
                           0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC,
                           0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6,
                           0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B};
    const uint8_t order[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
                               0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t zero[32] = { 0 };

    res = ec_ws_new_context(&ec_ctx, modulus, b, order, 32, 0);
    assert(res == 0);
    res = ec_ws_new_point(NULL, Gx, Gy, 32, ec_ctx);
    assert(res == ERR_NULL);
    res = ec_ws_new_point(&ecp, NULL, Gy, 32, ec_ctx);
    assert(res == ERR_NULL);
    res = ec_ws_new_point(&ecp, Gx, NULL, 32, ec_ctx);
    assert(res == ERR_NULL);
    res = ec_ws_new_point(&ecp, Gx, Gy, 32, NULL);
    assert(res == ERR_NULL);

    res = ec_ws_new_point(&ecp, Gx, Gy, 0, ec_ctx);
    assert(res == ERR_NOT_ENOUGH_DATA);

    res = ec_ws_new_point(&ecp, Gx_wrong, Gy, 32, ec_ctx);
    assert(res == ERR_EC_POINT);

    res = ec_ws_new_point(&ecp, Gx, Gy, 32, ec_ctx);
    assert(res == 0);

    ec_ws_free_point(ecp);
    res = ec_ws_new_point(&ecp, zero, zero, 32, ec_ctx);
    assert(res == 0);

    ec_ws_free_point(ecp);
    ec_free_context(ec_ctx);
}

void test_ec_ws_get_xy(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp;
    int res;
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    const uint8_t b[32] = {0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7,
                           0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC,
                           0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6,
                           0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B};
    const uint8_t order[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
                               0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t bufx[32], bufy[32];

    res = ec_ws_new_context(&ec_ctx, modulus, b, order, 32, 0);
    assert(res == 0);
    res = ec_ws_new_point(&ecp, Gx, Gy, 32, ec_ctx);
    assert(res == 0);
    assert(ecp != NULL);

    res = ec_ws_get_xy(NULL, bufy, 32, ecp);
    assert(res == ERR_NULL);
    res = ec_ws_get_xy(bufx, NULL, 32, ecp);
    assert(res == ERR_NULL);
    res = ec_ws_get_xy(bufx, bufy, 32, NULL);
    assert(res == ERR_NULL);

    res = ec_ws_get_xy(bufx, bufy, 31, ecp);
    assert(res == ERR_NOT_ENOUGH_DATA);

    res = ec_ws_get_xy(bufx, bufy, 32, ecp);
    assert(res == 0);

    assert(0 == memcmp(bufx, Gx, 32));
    assert(0 == memcmp(bufy, Gy, 32));

    ec_ws_free_point(ecp);
    ec_free_context(ec_ctx);
}

void test_ec_ws_double_p256(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp;
    int res;
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    const uint8_t b[32] = {0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7,
                           0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC,
                           0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6,
                           0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B};
    const uint8_t order[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
                               0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t bufx[32], bufy[32];

    ec_ws_new_context(&ec_ctx, modulus, b, order, 32, 0);
    ec_ws_new_point(&ecp, Gx, Gy, 32, ec_ctx);

    res = ec_ws_double(NULL);
    assert(res == ERR_NULL);

    res = ec_ws_double(ecp);
    assert(res == 0);
    ec_ws_get_xy(bufx, bufy, 32, ecp);
    assert(0 == memcmp(bufx, "\x7c\xf2\x7b\x18\x8d\x03\x4f\x7e\x8a\x52\x38\x03\x04\xb5\x1a\xc3\xc0\x89\x69\xe2\x77\xf2\x1b\x35\xa6\x0b\x48\xfc\x47\x66\x99\x78", 32));
    assert(0 == memcmp(bufy, "\x07\x77\x55\x10\xdb\x8e\xd0\x40\x29\x3d\x9a\xc6\x9f\x74\x30\xdb\xba\x7d\xad\xe6\x3c\xe9\x82\x29\x9e\x04\xb7\x9d\x22\x78\x73\xd1", 32));

    ec_ws_free_point(ecp);
    ec_free_context(ec_ctx);
}

void test_ec_ws_double_p521(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp;
    int res;
    const uint8_t Px[66] = {0x01, 0xD5, 0xC6, 0x93, 0xF6, 0x6C, 0x08, 0xED,
                            0x03, 0xAD, 0x0F, 0x03, 0x1F, 0x93, 0x74, 0x43,
                            0x45, 0x8F, 0x60, 0x1F, 0xD0, 0x98, 0xD3, 0xD0,
                            0x22, 0x7B, 0x4B, 0xF6, 0x28, 0x73, 0xAF, 0x50,
                            0x74, 0x0B, 0x0B, 0xB8, 0x4A, 0xA1, 0x57, 0xFC,
                            0x84, 0x7B, 0xCF, 0x8D, 0xC1, 0x6A, 0x8B, 0x2B,
                            0x8B, 0xFD, 0x8E, 0x2D, 0x0A, 0x7D, 0x39, 0xAF,
                            0x04, 0xB0, 0x89, 0x93, 0x0E, 0xF6, 0xDA, 0xD5,
                            0xC1, 0xB4};
    const uint8_t Py[66] = {0x01, 0x44, 0xB7, 0x77, 0x09, 0x63, 0xC6, 0x3A,
                            0x39, 0x24, 0x88, 0x65, 0xFF, 0x36, 0xB0, 0x74,
                            0x15, 0x1E, 0xAC, 0x33, 0x54, 0x9B, 0x22, 0x4A,
                            0xF5, 0xC8, 0x66, 0x4C, 0x54, 0x01, 0x2B, 0x81,
                            0x8E, 0xD0, 0x37, 0xB2, 0xB7, 0xC1, 0xA6, 0x3A,
                            0xC8, 0x9E, 0xBA, 0xA1, 0x1E, 0x07, 0xDB, 0x89,
                            0xFC, 0xEE, 0x5B, 0x55, 0x6E, 0x49, 0x76, 0x4E,
                            0xE3, 0xFA, 0x66, 0xEA, 0x7A, 0xE6, 0x1A, 0xC0,
                            0x18, 0x23};
    const uint8_t b[66] = {0x00, 0x51, 0x95, 0x3E, 0xB9, 0x61, 0x8E, 0x1C,
                           0x9A, 0x1F, 0x92, 0x9A, 0x21, 0xA0, 0xB6, 0x85,
                           0x40, 0xEE, 0xA2, 0xDA, 0x72, 0x5B, 0x99, 0xB3,
                           0x15, 0xF3, 0xB8, 0xB4, 0x89, 0x91, 0x8E, 0xF1,
                           0x09, 0xE1, 0x56, 0x19, 0x39, 0x51, 0xEC, 0x7E,
                           0x93, 0x7B, 0x16, 0x52, 0xC0, 0xBD, 0x3B, 0xB1,
                           0xBF, 0x07, 0x35, 0x73, 0xDF, 0x88, 0x3D, 0x2C,
                           0x34, 0xF1, 0xEF, 0x45, 0x1F, 0xD4, 0x6B, 0x50,
                           0x3F, 0x00};
    const uint8_t order[66] = {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xFF, 0xFA, 0x51, 0x86, 0x87, 0x83, 0xBF, 0x2F,
                               0x96, 0x6B, 0x7F, 0xCC, 0x01, 0x48, 0xF7, 0x09,
                               0xA5, 0xD0, 0x3B, 0xB5, 0xC9, 0xB8, 0x89, 0x9C,
                               0x47, 0xAE, 0xBB, 0x6F, 0xB7, 0x1E, 0x91, 0x38,
                               0x64, 0x09};
    const uint8_t modulus[66] = {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF};
    uint8_t bufx[66], bufy[66];

    ec_ws_new_context(&ec_ctx, modulus, b, order, 66, 0);
    ec_ws_new_point(&ecp, Px, Py, 66, ec_ctx);

    res = ec_ws_double(NULL);
    assert(res == ERR_NULL);

    res = ec_ws_double(ecp);
    assert(res == 0);
    ec_ws_get_xy(bufx, bufy, 66, ecp);
    assert(0 == memcmp(bufx, "\x01\x28\x79\x44\x2F\x24\x50\xC1\x19\xE7\x11\x9A\x5F\x73\x8B\xE1\xF1\xEB\xA9\xE9\xD7\xC6\xCF\x41\xB3\x25\xD9\xCE\x6D\x64\x31\x06\xE9\xD6\x11\x24\xA9\x1A\x96\xBC\xF2\x01\x30\x5A\x9D\xEE\x55\xFA\x79\x13\x6D\xC7\x00\x83\x1E\x54\xC3\xCA\x4F\xF2\x64\x6B\xD3\xC3\x6B\xC6", 66));
    assert(0 == memcmp(bufy, "\x01\x98\x64\xA8\xB8\x85\x5C\x24\x79\xCB\xEF\xE3\x75\xAE\x55\x3E\x23\x93\x27\x1E\xD3\x6F\xAD\xFC\x44\x94\xFC\x05\x83\xF6\xBD\x03\x59\x88\x96\xF3\x98\x54\xAB\xEA\xE5\xF9\xA6\x51\x5A\x02\x1E\x2C\x0E\xEF\x13\x9E\x71\xDE\x61\x01\x43\xF5\x33\x82\xF4\x10\x4D\xCC\xB5\x43", 66));

    ec_ws_free_point(ecp);
    ec_free_context(ec_ctx);
}

void test_ec_ws_add(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp, *ecp2;
    int res;
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    const uint8_t b[32] = {0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7,
                           0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC,
                           0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6,
                           0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B};
    const uint8_t order[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
                               0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t bufx[32], bufy[32];

    ec_ws_new_context(&ec_ctx, modulus, b, order, 32, 0);
    ec_ws_new_point(&ecp, Gx, Gy, 32, ec_ctx);
    ec_ws_new_point(&ecp2, Gx, Gy, 32, ec_ctx);
    ec_ws_double(ecp2);

    res = ec_ws_add(NULL, ecp);
    assert(res == ERR_NULL);
    res = ec_ws_add(ecp, NULL);
    assert(res == ERR_NULL);

    res = ec_ws_add(ecp, ecp2);
    assert(res == 0);
    ec_ws_get_xy(bufx, bufy, 32, ecp);
    assert(0 == memcmp(bufx, "\x5e\xcb\xe4\xd1\xa6\x33\x0a\x44\xc8\xf7\xef\x95\x1d\x4b\xf1\x65\xe6\xc6\xb7\x21\xef\xad\xa9\x85\xfb\x41\x66\x1b\xc6\xe7\xfd\x6c", 32));
    assert(0 == memcmp(bufy, "\x87\x34\x64\x0c\x49\x98\xff\x7e\x37\x4b\x06\xce\x1a\x64\xa2\xec\xd8\x2a\xb0\x36\x38\x4f\xb8\x3d\x9a\x79\xb1\x27\xa2\x7d\x50\x32", 32));

    ec_ws_free_point(ecp);
    ec_ws_free_point(ecp2);
    ec_free_context(ec_ctx);
}

void test_ec_ws_scalar(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp;
    int res;
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    const uint8_t b[32] = {0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7,
                           0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC,
                           0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6,
                           0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B};
    const uint8_t order[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
                               0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t bufx[32], bufy[32];

    ec_ws_new_context(&ec_ctx, modulus, b, order, 32, 0x5EED);
    ec_ws_new_point(&ecp, Gx, Gy, 32, ec_ctx);

    res = ec_ws_scalar(NULL, (uint8_t*)"\xFF\xFF", 2, 0xFFFF);
    assert(res == ERR_NULL);
    res = ec_ws_scalar(ecp, NULL, 2, 0xFFFF);
    assert(res == ERR_NULL);

    res = ec_ws_scalar(ecp, (uint8_t*)"\xFF\xFF", 2, 0xFFFF);
    assert(res == 0);
    ec_ws_get_xy(bufx, bufy, 32, ecp);
    assert(0 == memcmp(bufx, "\xf2\x49\x10\x4d\x0e\x6f\x8f\x29\xe6\x01\x62\x77\x78\x0c\xda\x84\xdc\x84\xb8\x3b\xc3\xd8\x99\xdf\xb7\x36\xca\x08\x31\xfb\xe8\xcf", 32));
    assert(0 == memcmp(bufy, "\xb5\x7e\x12\xfc\xdb\x03\x1f\x59\xca\xb8\x1b\x1c\x6b\x1e\x1c\x07\xe4\x51\x2e\x52\xce\x83\x2f\x1a\x0c\xed\xef\xff\x8b\x43\x40\xe9", 32));

    ec_ws_free_point(ecp);
    ec_free_context(ec_ctx);
}

void test_ec_ws_neg(void)
{
    EcContext *ec_ctx;
    EcPoint *ecp;
    int res;
    const uint8_t Gx[32] = {0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
                            0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
                            0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
                            0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96};
    const uint8_t Gy[32] = {0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
                            0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
                            0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
                            0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5};
    const uint8_t b[32] = {0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7,
                           0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC,
                           0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6,
                           0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B};
    const uint8_t order[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                               0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                               0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
                               0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
    const uint8_t modulus[32] = {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t bufx[32], bufy[32];

    ec_ws_new_context(&ec_ctx, modulus, b, order, 32, 0);
    ec_ws_new_point(&ecp, Gx, Gy, 32, ec_ctx);

    res = ec_ws_neg(NULL);
    assert(res == ERR_NULL);

    res = ec_ws_neg(ecp);
    assert(res == 0);
    ec_ws_get_xy(bufx, bufy, 32, ecp);
    assert(0 == memcmp(bufx, "\x6b\x17\xd1\xf2\xe1\x2c\x42\x47\xf8\xbc\xe6\xe5\x63\xa4\x40\xf2\x77\x03\x7d\x81\x2d\xeb\x33\xa0\xf4\xa1\x39\x45\xd8\x98\xc2\x96", 32));
    assert(0 == memcmp(bufy, "\xb0\x1c\xbd\x1c\x01\xe5\x80\x65\x71\x18\x14\xb5\x83\xf0\x61\xe9\xd4\x31\xcc\xa9\x94\xce\xa1\x31\x34\x49\xbf\x97\xc8\x40\xae\x0a", 32));

    ec_ws_free_point(ecp);
    ec_free_context(ec_ctx);
}


int main(void) {
    test_ec_projective_to_affine();
    test_ec_full_double();
    test_ec_mix_add();
    test_ec_full_add();
    test_ec_scalar();
    test_ec_scalar_g_p256();
    test_ec_ws_new_point();
    test_ec_ws_get_xy();
    test_ec_ws_double_p256();
    test_ec_ws_double_p521();
    test_ec_ws_add();
    test_ec_ws_scalar();
    test_ec_ws_neg();
    return 0;
}
