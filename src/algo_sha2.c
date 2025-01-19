/* Include ----------------------------------------------------------------- */
#include "algo_sha2.h"

/* Define ----------------------------------------------------------------- */
#define SHR(x, n)  (((x) & 0xFFFFFFFF) >> (n))
#define ROTR(x, n) (SHR(x, n) | ((x) << (32 - (n))))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) (((x) & (y)) | ((z) & ((x) | (y))))
#define F1(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))

/* Variable ---------------------------------------------------------------- */
static const uint32_t K[] = {
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

/* Function ----------------------------------------------------------------- */
static void SHA256_SHA224_Process(ALGO_SHA256_Context_t *ctx, const uint8_t data[64])
{
    struct {
        uint32_t temp1, temp2, W[64];
        uint32_t A[8];
    } local;

    uint32_t i;

    for (i = 0; i < 8; i++) {
        local.A[i] = ctx->state[i];
    }

    for (i = 0; i < 64; i++) {
        if (i < 16) {
            local.W[i] = ALGO_GetU32BE(&(data[sizeof(uint32_t) * i]));
        } else {
            local.W[i] = S1(local.W[i - 2]) + local.W[i - 7] + S0(local.W[i - 15]) + local.W[i - 16];
        }

        local.temp1 = local.A[7] + S3(local.A[4]) + F1(local.A[4], local.A[5], local.A[6]) + K[i] + local.W[i];
        local.temp2 = S2(local.A[0]) + F0((local.A[0]), (local.A[1]), (local.A[2]));
        local.A[3] += local.temp1;
        local.A[7] = local.temp1 + local.temp2;

        local.temp1 = local.A[7];
        local.A[7] = local.A[6];
        local.A[6] = local.A[5];
        local.A[5] = local.A[4];
        local.A[4] = local.A[3];
        local.A[3] = local.A[2];
        local.A[2] = local.A[1];
        local.A[1] = local.A[0];
        local.A[0] = local.temp1;
    }

    for (i = 0; i < 8; i++) {
        ctx->state[i] += local.A[i];
    }
}

static void ALGO_SHA256_SHA224_Finish(ALGO_SHA256_Context_t *ctx, uint8_t *hash, size_t size)
{
    uint32_t used = ctx->total[0x00] & 0x3F;
    ctx->buff[used++] = 0x80;

    if (used <= 56) {
        memset(&(ctx->buff[used]), 0, 56 - used);
    } else {
        memset(&(ctx->buff[used]), 0, 64 - used);

        SHA256_SHA224_Process(ctx, ctx->buff);

        memset(&(ctx->buff[0]), 0, 56);
    }

    ALGO_PutU32BE(&(ctx->buff[56]), (ctx->total[0] >> 29) | (ctx->total[1] << 3));
    ALGO_PutU32BE(&(ctx->buff[60]), (ctx->total[0] << 3));

    SHA256_SHA224_Process(ctx, ctx->buff);

    for (size_t i = 0; i < (size / sizeof(uint32_t)); i++) {
        ALGO_PutU32BE(&(hash[sizeof(uint32_t) * i]), ctx->state[i]);
    }
}

void ALGO_SHA256_Init(ALGO_SHA256_Context_t *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0x6A09E667;
    ctx->state[1] = 0xBB67AE85;
    ctx->state[2] = 0x3C6EF372;
    ctx->state[3] = 0xA54FF53A;
    ctx->state[4] = 0x510E527F;
    ctx->state[5] = 0x9B05688C;
    ctx->state[6] = 0x1F83D9AB;
    ctx->state[7] = 0x5BE0CD19;
}

void ALGO_SHA256_Update(ALGO_SHA256_Context_t *ctx, const uint8_t *data, size_t size)
{
    uint32_t left = ctx->total[0] & 0x3F;
    uint32_t fill = 64 - left;

    ctx->total[0] += (uint32_t)size;
    ctx->total[0] &= 0xFFFFFFFF;

    if (ctx->total[0] < (uint32_t)size) {
        ctx->total[1]++;
    }

    if ((left > 0) && (size >= fill)) {
        memcpy(&(ctx->buff[left]), data, fill);

        SHA256_SHA224_Process(ctx, ctx->buff);

        data += fill;
        size -= fill;
        left = 0;
    }

    while (size >= 64) {
        SHA256_SHA224_Process(ctx, data);
        data += 64;
        size -= 64;
    }

    if (size > 0) {
        memcpy(&(ctx->buff[left]), data, size);
    }
}

void ALGO_SHA256_Finish(ALGO_SHA256_Context_t *ctx, ALGO_SHA256_Digest_t *digest)
{
    ALGO_SHA256_SHA224_Finish(ctx, digest->hash, sizeof(digest->hash));
}

void ALGO_SHA256_Calulate(const uint8_t *data, size_t len, ALGO_SHA256_Digest_t *digest)
{
    ALGO_SHA256_Context_t ctx;

    ALGO_SHA256_Init(&ctx);
    ALGO_SHA256_Update(&ctx, data, len);
    ALGO_SHA256_Finish(&ctx, digest);
}

void ALGO_SHA224_Init(ALGO_SHA224_Context_t *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;

    ctx->state[0] = 0xC1059ED8;
    ctx->state[1] = 0x367CD507;
    ctx->state[2] = 0x3070DD17;
    ctx->state[3] = 0xF70E5939;
    ctx->state[4] = 0xFFC00B31;
    ctx->state[5] = 0x68581511;
    ctx->state[6] = 0x64F98FA7;
    ctx->state[7] = 0xBEFA4FA4;
}

void ALGO_SHA224_Update(ALGO_SHA224_Context_t *ctx, const uint8_t *data, size_t size)
{
    ALGO_SHA256_Update(ctx, data, size);
}

void ALGO_SHA224_Finish(ALGO_SHA224_Context_t *ctx, ALGO_SHA224_Digest_t *digest)
{
    ALGO_SHA256_SHA224_Finish(ctx, digest->hash, sizeof(digest->hash));
}

void ALGO_SHA224_Calulate(const uint8_t *data, size_t len, ALGO_SHA224_Digest_t *digest)
{
    ALGO_SHA224_Context_t ctx;

    ALGO_SHA224_Init(&ctx);
    ALGO_SHA224_Update(&ctx, data, len);
    ALGO_SHA224_Finish(&ctx, digest);
}
