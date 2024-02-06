#include <iostream>
#include <string>
#include <bitset>
#include "qr.hpp"

// Add the missing header file that defines the "ver" identifier

void sha1Transform(uint32_t state[5], const unsigned char block[64]);
std::string sha1(const std::string& input);
std::string generateRandomSecretKey();
std::string generateOTP(const std::string& key, const long timeStep = 30, const int digits = 6);
bool validateOTP(const std::string& key, const std::string& userEnteredOTP, const long timeStep = 30, const int digits = 6); 
template <typename T>
void printQr(const T &codec); 