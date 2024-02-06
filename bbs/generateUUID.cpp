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
/*2024 zaja driving chatGPT*/

#include <iostream>
#include <string>
#include <sstream>
#include <random>
#include <bitset>

std::string generateUUID() {
    // Random number generator for UUID generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    // UUID format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    const char* uuidTemplate = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    std::ostringstream uuidStream;

    for (int i = 0; uuidTemplate[i] != '\0'; ++i) {
        if (uuidTemplate[i] == 'x') {
            uuidStream << std::hex << dis(gen);
        } else if (uuidTemplate[i] == 'y') {
            uuidStream << std::hex << (dis(gen) & 0x3 | 0x8); // Ensure '4' at the third position
        } else {
            uuidStream << uuidTemplate[i];
        }
    }

    return uuidStream.str();
}