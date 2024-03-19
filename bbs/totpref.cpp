#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cstdint>

#define ALLOWED_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
#define LENGTH 32

std::string generateKey() {
    const char allowed_chars[] = ALLOWED_CHARS;
    std::string generated_string;

    srand(time(NULL));

    for (int i = 0; i < LENGTH; ++i) {
        int random_index = rand() % (sizeof(allowed_chars) - 1);
        generated_string += allowed_chars[random_index];
    }

    return generated_string;
}

static const int8_t base32_vals[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    14, 11, 26, 27, 28, 29, 30, 31, 1,  -1, -1, -1, -1, 0,  -1, -1,
    -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, -1, -1, -1, -1
};

int generateOtp(const std::string& key) {
    size_t pos;
    size_t len = key.length();
    size_t keylen;
    uint64_t x = 30;
    uint64_t t0 = 0;
    uint64_t t;
    uint64_t offset;
    uint8_t* hmac_result;
    uint32_t bin_code;
    uint32_t totp;
    uint8_t k[LENGTH]; // user's secret key

    for (pos = 0; pos < len; pos++) {
        if (base32_vals[(int)key[pos]] == -1) {
            fprintf(stderr, "Invalid base32 secret\n");
            return 1;
        }
    }

    keylen = 0;
    for (pos = 0; pos <= (len - 8); pos += 8) {
        k[keylen + 0] = (base32_vals[key[pos + 0]] << 3) & 0xF8;
        k[keylen + 0] |= (base32_vals[key[pos + 1]] >> 2) & 0x07;
        if (key[pos + 2] == '=') {
            keylen += 1;
            break;
        }
        k[keylen + 1] = (base32_vals[key[pos + 1]] << 6) & 0xC0;
        k[keylen + 1] |= (base32_vals[key[pos + 2]] << 1) & 0x3E;
        k[keylen + 1] |= (base32_vals[key[pos + 3]] >> 4) & 0x01;
        if (key[pos + 4] == '=') {
            keylen += 2;
            break;
        }
        k[keylen + 2] = (base32_vals[key[pos + 3]] << 4) & 0xF0;
        k[keylen + 2] |= (base32_vals[key[pos + 4]] >> 1) & 0x0F;
        if (key[pos + 5] == '=') {
            keylen += 3;
            break;
        }
        k[keylen + 3] = (base32_vals[key[pos + 4]] << 7) & 0x80;
        k[keylen + 3] |= (base32_vals[key[pos + 5]] << 2) & 0x7C;
        k[keylen + 3] |= (base32_vals[key[pos + 6]] >> 3) & 0x03;
        if (key[pos + 7] == '=') {
            keylen += 4;
            break;
        }
        k[keylen + 4] = (base32_vals[key[pos + 6]] << 5) & 0xE0;
        k[keylen + 4] |= (base32_vals[key[pos + 7]] >> 0) & 0x1F;
        keylen += 5;
    }
    k[keylen] = 0;

    t = (time(NULL) - t0) / x;

    hmac_result = (uint8_t*)HMAC(EVP_sha1(), k, keylen, (const unsigned char*)&t, sizeof(t), NULL, 0);

    offset = hmac_result[19] & 0x0f;
    bin_code = (hmac_result[offset] & 0x7f) << 24 | (hmac_result[offset + 1] & 0xff) << 16 |
               (hmac_result[offset + 2] & 0xff) << 8 | (hmac_result[offset + 3] & 0xff);

    totp = bin_code % 1000000;

    return totp;
}

int main(int argc, char *argv[]) {
    std::string key;
    if (argc > 1) {
        // If a key is provided as a command-line argument, use it
        key = argv[1];
    } else {
        // Otherwise, generate a new key
        key = generateKey();
        printf("Generated Key: %s\n", key.c_str());
    }
    
    int otp = generateOtp(key);
    printf("Generated OTP: %06d\n", otp);
    
    return 0;
}
