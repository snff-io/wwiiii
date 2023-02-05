/**************************************************************************/
/*                                                                        */
/*                            WWIV Version 5                              */
/*             Copyright (C)2015-2022, WWIV Software Services             */
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
#ifndef INCLUDED_SDK_SUBSCRIBERS_H
#define INCLUDED_SDK_SUBSCRIBERS_H

#include "sdk/fido/fido_address.h"
#include <filesystem>
#include <set>

namespace wwiv::sdk {

std::set<fido::FidoAddress> ReadFidoSubcriberFile(const std::filesystem::path& filename);
std::set<uint16_t> ReadSubcriberFile(const std::filesystem::path& path);
bool WriteSubcriberFile(const std::filesystem::path& path, const std::set<uint16_t>& subscribers);
bool WriteFidoSubcriberFile(const std::filesystem::path& path,
                            const std::set<fido::FidoAddress>& subscribers);

} // namespace wwiv::sdk

#endif
