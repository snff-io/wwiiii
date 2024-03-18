#include <iostream>
#include <string>
#include <random>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/bio.h>

#define ALLOWED_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
#define LENGTH 32

std::string generate_key() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(ALLOWED_CHARS) - 2);

    std::string generated_string;
    generated_string.reserve(LENGTH);

    for (int i = 0; i < LENGTH; ++i) {
        generated_string += ALLOWED_CHARS[dis(gen)];
    }

    return generated_string;
}

int generate_otp(const std::string& key) {
    size_t keylen = key.length();
    uint64_t t;
    uint8_t hmac_result[SHA_DIGEST_LENGTH];

    // Converts T to big endian if system is little endian
    t = (time(NULL) - 0) / 30;
    uint32_t endianness = 0xdeadbeef;
    if (*(const uint8_t *)&endianness == 0xef) {
        t = ((t & 0x00000000ffffffff) << 32) | ((t & 0xffffffff00000000) >> 32);
        t = ((t & 0x0000ffff0000ffff) << 16) | ((t & 0xffff0000ffff0000) >> 16);
        t = ((t & 0x00ff00ff00ff00ff) <<  8) | ((t & 0xff00ff00ff00ff00) >>  8);
    }

    // Determines hash
    HMAC(EVP_sha1(), key.c_str(), keylen, (const unsigned char *)&t, sizeof(t), hmac_result, NULL);

    // Dynamically truncates hash
    int offset = hmac_result[SHA_DIGEST_LENGTH - 1] & 0x0f;
    int bin_code = (hmac_result[offset]  & 0x7f) << 24 |
                   (hmac_result[offset + 1] & 0xff) << 16 |
                   (hmac_result[offset + 2] & 0xff) <<  8 |
                   (hmac_result[offset + 3] & 0xff);

    // Truncates code to 6 digits
    int totp = bin_code % 1000000;

    return totp;
}
int main(int argc, char* argv[]) {
    std::string key;
    if (argc > 1) {
        key = argv[1];
    } else {
        key = generate_key();
        std::cout << "Generated key: " << key << std::endl;
    }
    int otp = generate_otp(key);
    std::cout << "Generated OTP: " << otp << std::endl;

    return 0;
}
