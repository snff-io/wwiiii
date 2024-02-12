/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*             Copyright (C)1998-2022, WWIV Software Services             */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/**************************************************************************/
/*2024 zaja driving chatgpt */


#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cmath>
#include <random>
#include <iostream>
#include <string>
#include <bitset>
#include "qr.hpp"

// Rotate left function
#define ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// SHA-1 functions
#define SHA1_CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define SHA1_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define SHA1_ROTL(x, n) ((x) << (n))

#define SHA1_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

#define SHA1_USIG0(x) (SHA1_ROTR(x, 2) ^ SHA1_ROTR(x, 13) ^ SHA1_ROTR(x, 22))
#define SHA1_USIG1(x) (SHA1_ROTR(x, 6) ^ SHA1_ROTR(x, 11) ^ SHA1_ROTR(x, 25))
#define SHA1_LSIG0(x) (SHA1_ROTR(x, 7) ^ SHA1_ROTR(x, 18) ^ ((x) >> 3))
#define SHA1_LSIG1(x) (SHA1_ROTR(x, 17) ^ SHA1_ROTR(x, 19) ^ ((x) >> 10))

void sha1Transform(uint32_t state[5], const unsigned char block[64]) {
    uint32_t W[80];

    // Prepare the message schedule
    for (int t = 0; t < 16; ++t) {
        W[t] = block[t * 4] << 24 | block[t * 4 + 1] << 16 | block[t * 4 + 2] << 8 | block[t * 4 + 3];
    }
    for (int t = 16; t < 80; ++t) {
        W[t] = ROTL32(W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16], 1);
    }

    // Initialize hash value for this chunk
    uint32_t A = state[0];
    uint32_t B = state[1];
    uint32_t C = state[2];
    uint32_t D = state[3];
    uint32_t E = state[4];

    // Main loop
    for (int t = 0; t < 80; ++t) {
        uint32_t temp = ROTL32(A, 5) + SHA1_CH(B, C, D) + E + W[t];
        temp += (t < 20) ? SHA1_USIG0(A) + ((B & C) | (~B & D)) : ((t < 40) ? SHA1_USIG1(A) + SHA1_MAJ(B, C, D) : ((t < 60) ? SHA1_LSIG0(A) + SHA1_MAJ(B, C, D) : SHA1_LSIG1(A) + SHA1_MAJ(B, C, D)));

        E = D;
        D = C;
        C = ROTL32(B, 30);
        B = A;
        A = temp;
    }

    // Add this chunk's hash to result so far
    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
    state[4] += E;
}

std::string sha1(const std::string& input) {
    // Initialize variables
    uint32_t state[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    uint64_t bitCount = input.length() * 8;

    // Pad the input to 512 bits
    std::string paddedInput = input;
    paddedInput += static_cast<char>(0x80);  // Append a single '1' bit
    while ((paddedInput.length() % 64) != 56) {
        paddedInput += static_cast<char>(0x00);  // Append '0' bits
    }

    // Append the bit count in big-endian format
    for (int i = 7; i >= 0; --i) {
        paddedInput += static_cast<char>((bitCount >> (i * 8)) & 0xFF);
    }

    // Process each 512-bit block
    for (size_t i = 0; i < paddedInput.length(); i += 64) {
        sha1Transform(state, reinterpret_cast<const unsigned char*>(&paddedInput[i]));
    }

    // Convert the result to a hexadecimal string
    std::ostringstream result;
    for (int i = 0; i < 5; ++i) {
        result << std::hex << std::setfill('0') << std::setw(8) << state[i];
    }

    return result.str();
}

std::string generateOTP(const std::string& key, const long timeStep = 30, const int digits = 6) {
    // Get the current time in seconds since the Unix epoch
    long currentTime = std::time(nullptr);

    // Calculate the counter based on the time step
    long counter = currentTime / timeStep;

    // Convert the counter to a byte array (big-endian)
    unsigned char counterBytes[8];
    for (int i = 7; i >= 0; --i) {
        counterBytes[i] = static_cast<unsigned char>((counter >> (i * 8)) & 0xFF);
    }

    // Use sha1 function to calculate the hash
    std::string hashValue = sha1(key + std::string(reinterpret_cast<char*>(counterBytes), 8));

    // Take the least significant 4 bytes from the hashValue
    int offset = hashValue[hashValue.length() - 1] & 0xF;
    int binary =
        ((hashValue[offset] & 0x7F) << 24) |
        ((hashValue[offset + 1] & 0xFF) << 16) |
        ((hashValue[offset + 2] & 0xFF) << 8) |
        (hashValue[offset + 3] & 0xFF);

    // Modulo to get a 6-digit OTP
    int otpValue = binary % static_cast<int>(std::pow(10, digits));

    // Format the OTP with leading zeros if necessary
    std::ostringstream otpStream;
    otpStream << std::setw(digits) << std::setfill('0') << otpValue;

    return otpStream.str();
}

bool validateOTP(const std::string& key, const std::string& userEnteredOTP, const long timeStep = 30, const int digits = 6) {
    // Generate the expected OTP using the same parameters
    std::string expectedOTP = generateOTP(key, timeStep, digits);

    // Compare the user-entered OTP with the expected OTP
    return (userEnteredOTP == expectedOTP);
}

std::string generateRandomSecretKey() {
    // Generate a random 10-digit number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(100000000, 999999999); // Range for 10-digit number

    return std::to_string(dis(gen));
}

constexpr int ver = 4;
constexpr auto ecc = qr::Ecc::H;

void printQr(const qr::Qr<ver> &codec) {
    printf("\n\n");
    for (int y = 0; y < codec.side_size(); ++y) {
        printf("        ");
        for (int x = 0; x < codec.side_size(); ++x)
            printf(codec.module(x, y) ? "\u2588\u2588" : "  ");
        printf("\n");
    }
    printf("\n\n");
}

int main( ) {

    // Replace "YourSecretKey" with your secret key
    std::string secretKey = "23423rded"; //generateRandomSecretKey();
    std::cout << "Generated Secret: " << secretKey << std::endl;
    std::cout << "SHA1 of secret: " << sha1(secretKey) << std::endl;
    // Generate and display the micro QR code
   std::string str = "https://the.worldcomputer.info"; 
   //secretKey;

    qr::Qr<ver> qr;
    qr.encode(str.c_str(), str.length(), ecc, 0);
    printQr(qr);
    // Generate and display the OTP

    std::string otp = generateOTP(secretKey);
    std::cout << "Generated OTP: " << otp << std::endl;

    // Simulate user entering OTP (replace with actual user input)
    std::string userEnteredOTP;
    std::cout << "Enter the OTP: ";
    std::cin >> userEnteredOTP;

    // Validate the entered OTP
    if (validateOTP(secretKey, userEnteredOTP)) {
        std::cout << "OTP is valid!" << std::endl;
    }
    else {
        std::cout << "OTP is invalid!" << std::endl;
    }
}
        