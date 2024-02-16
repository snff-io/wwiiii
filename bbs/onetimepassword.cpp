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
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include "qr.hpp"
#include "totp.c"

bool validateOTP(const std::string& key, const std::string& userEnteredOTP, const long timeStep = 30, const int digits = 6) {
    // Generate the expected OTP using the same parameters
    std::string expectedOTP = generateOTP(key, timeStep, digits);

    // Compare the user-entered OTP with the expected OTP
    return (userEnteredOTP == expectedOTP);
}

constexpr int ver = 4;
constexpr auto ecc = qr::Ecc::H;

std:string printQrKey(std::string key) {
    qr::Qr<ver> qr;)
    
    qr.encode(key.c_str(), key.length(), ecc, 0);
    
    printQr(qr);

    printf("\n\n");
    
    for (int y = 0; y < qr.side_size(); ++y) {
        printf("        ");
        for (int x = 0; x < qr.side_size(); ++x)
            printf(qr.module(x, y) ? "\u2588\u2588" : "  ");
        printf("\n");
    }
    printf("\n\n");

    return secret;
}



int main(int argc, char * argv[]) {


    std::string key = generateKey();
    std::cout << "Generated Key: " << key << std::endl;
    printQrKey(key);


    std::string otp = generateOTP(key);
    std::cout << "Generated OTP: " << otp << std::endl;

    // Simulate user entering OTP (replace with actual user input)
    std::string userEnteredOTP;
    std::cout << "Enter the OTP: ";
    std::cin >> userEnteredOTP;

    // Validate the entered OTP
    if (validateOTP(key, userEnteredOTP)) {
        std::cout << "OTP is valid!" << std::endl;
    }
    else {
        std::cout << "OTP is invalid!" << std::endl;
    }
}
        