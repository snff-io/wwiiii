#include <ctime>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <string>

#define ALLOWED_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
#define LENGTH 32

// Function to decode base32-encoded string
void base32_decode(const std::string& encoded, uint8_t* decoded) {
  static const int8_t base32_vals[256] = {
      //    This map cheats and interprets:
      //       - the numeral zero as the letter "O" as in oscar
      //       - the numeral one as the letter "L" as in lima
      //       - the numeral eight as the letter "B" as in bravo
      // 00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x00
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x10
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x20
      14, 11, 26, 27, 28, 29, 30, 31, 1,  -1, -1, -1, -1, 0,  -1, -1, // 0x30
      -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, // 0x40
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, // 0x50
      -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, // 0x60
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, -1, -1, -1, -1, // 0x70
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x80
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0x90
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0xA0
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0xB0
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0xC0
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0xD0
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0xE0
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0xF0
  };

  size_t keylen = 0;
  size_t len = encoded.length();
  for (size_t pos = 0; pos <= (len - 8); pos += 8) {
    decoded[keylen + 0] = (base32_vals[encoded[pos + 0]] << 3) & 0xF8;  // 5 MSB
    decoded[keylen + 0] |= (base32_vals[encoded[pos + 1]] >> 2) & 0x07; // 3 LSB

    if (encoded[pos + 2] == '=') {
      keylen += 1;
      break;
    }

    decoded[keylen + 1] = (base32_vals[encoded[pos + 1]] << 6) & 0xC0;  // 2 MSB
    decoded[keylen + 1] |= (base32_vals[encoded[pos + 2]] << 1) & 0x3E; // 5 MB
    decoded[keylen + 1] |= (base32_vals[encoded[pos + 3]] >> 4) & 0x01; // 1 LSB

    if (encoded[pos + 4] == '=') {
      keylen += 2;
      break;
    }

    decoded[keylen + 2] = (base32_vals[encoded[pos + 3]] << 4) & 0xF0;  // 4 MSB
    decoded[keylen + 2] |= (base32_vals[encoded[pos + 4]] >> 1) & 0x0F; // 4 LSB

    if (encoded[pos + 5] == '=') {
      keylen += 3;
      break;
    }

    decoded[keylen + 3] = (base32_vals[encoded[pos + 4]] << 7) & 0x80;  // 1 MSB
    decoded[keylen + 3] |= (base32_vals[encoded[pos + 5]] << 2) & 0x7C; // 5 MB
    decoded[keylen + 3] |= (base32_vals[encoded[pos + 6]] >> 3) & 0x03; // 2 LSB

    if (encoded[pos + 7] == '=') {
      keylen += 4;
      break;
    }

    decoded[keylen + 4] = (base32_vals[encoded[pos + 6]] << 5) & 0xE0;  // 3 MSB
    decoded[keylen + 4] |= (base32_vals[encoded[pos + 7]] >> 0) & 0x1F; // 5 LSB
    keylen += 5;
  }
}

std::string generate_key() {
  char allowed_chars[] = ALLOWED_CHARS;
  std::string generated_string;

  srand(time(NULL));

  for (int i = 0; i < LENGTH; ++i) {
    int random_index = rand() % (sizeof(allowed_chars) - 1);
    generated_string += allowed_chars[random_index];
  }

  return generated_string;
}

int generate_otp(const std::string& key) {
  uint8_t decoded_key[LENGTH];
  base32_decode(key, decoded_key);

  uint64_t t, offset, bin_code;
  uint8_t* hmac_result;

  // Ensure t is in big-endian format
  t = (time(NULL) - 0) / 30;
  t = htobe64(t);

  // Determines hash
  hmac_result = (uint8_t *)HMAC(EVP_sha1(), decoded_key, LENGTH, (const unsigned char*)&t, sizeof(t), NULL, 0);

  // Dynamically truncates hash
  offset = hmac_result[19] & 0x0f;
  bin_code = (hmac_result[offset] & 0x7f) << 24 | (hmac_result[offset + 1] & 0xff) << 16 |
             (hmac_result[offset + 2] & 0xff) << 8 | (hmac_result[offset + 3] & 0xff);

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
