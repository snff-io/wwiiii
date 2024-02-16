#include <iostream>
#include <ctime>
#include <cstring>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include "qr.hpp"

// Function to generate a base32 key
std::string generateBase32Key(int length) {
    static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    std::string key;
    for (int i = 0; i < length; ++i) {
        key += alphabet[rand() % alphabet.length()];
    }
    return key;
}

// Function to generate TOTP (Time-Based One-Time Password)
std::string generateOTP(const std::string &key, uint64_t timestamp, int timeStep = 30, int digit = 6) {
    // Convert timestamp to time steps
    timestamp /= timeStep;
    uint8_t data[8];
    for (int i = 8; i--;)
        data[i] = (timestamp >> (i * 8)) & 0xff;

    // HMAC-SHA1 calculation
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    HMAC(EVP_sha1(), key.c_str(), key.length(), data, sizeof(data), hash, &hashLen);

    // Dynamic Truncation
    unsigned int offset = hash[hashLen - 1] & 0xf;
    unsigned int binary =
        ((hash[offset] & 0x7f) << 24) |
        ((hash[offset + 1] & 0xff) << 16) |
        ((hash[offset + 2] & 0xff) << 8) |
        (hash[offset + 3] & 0xff);

    // Generate the OTP
    std::string otp;
    for (int i = 0; i < digit; ++i) {
        otp += '0' + (binary % 10);
        binary /= 10;
    }

    return otp; // Reverse the OTP
}

bool validateOTP(const std::string &key, const std::string &otp, uint64_t timestamp, int timeStep = 30, int window = 1) {
    // Check OTPs for a window of time steps
    for (int i = 0; i < window; ++i) {
        std::string expectedOTP = generateOTP(key, timestamp + i * timeStep);
        if (otp == expectedOTP) {
            return true;
        }
    }
    return false;
}

// Function to calculate the time remaining until the next OTP generation
int timeUntilNextOTP(int timeStep) {
    time_t now = std::time(nullptr);
    return timeStep - (now % timeStep);
}
constexpr int ver = 4;
constexpr auto ecc = qr::Ecc::H;

std::string printQrKey(std::string key) {
    qr::Qr<ver> qr;
    
    qr.encode(key.c_str(), key.length(), ecc, 0);
    
    //printQr(qr);

    printf("\n\n");
    
    for (int y = 0; y < qr.side_size(); ++y) {
        printf("        ");
        for (int x = 0; x < qr.side_size(); ++x)
            printf(qr.module(x, y) ? "\u2588\u2588" : "  ");
        printf("\n");
    }
    printf("\n\n");

    return key;
}

int main() {
    srand(time(nullptr));
    std::string key = generateBase32Key(16); // Generate a 16-character base32 key
    //TODO: fix
    key = "X3MAJP4B2WQ2YTYE";
    printQrKey(key);

    std::cout << "Generated Key: " << key << std::endl;

    int timeStep = 30; // Time step in seconds

    // Generate and print TOTP for current time
    uint64_t timestamp = std::time(nullptr);
    std::string otp = generateOTP(key, timestamp);
    std::cout << "OTP at current time: " << otp << std::endl;

    int timeRemaining = timeUntilNextOTP(timeStep);
    std::cout << "Time remaining until next OTP: " << timeRemaining << " seconds" << std::endl;

    if (validateOTP(key, otp, timestamp, timeStep)) {
        std::cout << "OTP is valid." << std::endl;
    } else {
        std::cout << "OTP is NOT valid." << std::endl;
    }

    // Simulate user entering OTP (replace with actual user input)
    std::string userEnteredOTP;
    std::cout << "Enter the OTP: ";
    std::cin >> userEnteredOTP;

    if (validateOTP(key, userEnteredOTP, timestamp, timeStep)) {
        std::cout << "User OTP is valid." << std::endl;
    } else {
        std::cout << "User OTP is NOT valid." << std::endl;
    }


    // Wait until the next time step
    //std::this_thread::sleep_for(std::chrono::seconds(timeRemaining));
    

    return 0;
}