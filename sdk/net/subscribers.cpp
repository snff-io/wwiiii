/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
/*             Copyright (C)2016-2022, WWIV Software Services             */
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
#include "sdk/net/subscribers.h"

#include "core/file.h"
#include "core/log.h"
#include "core/strings.h"
#include "core/textfile.h"
#include "sdk/fido/fido_address.h"
#include <map>
#include <set>
#include <string>

using namespace wwiv::core;
using namespace wwiv::strings;
using namespace wwiv::sdk::fido;

namespace wwiv::sdk {

std::set<FidoAddress> ReadFidoSubcriberFile(const std::filesystem::path& path) {

  VLOG(1) << "ReadFidoSubcriberFile: " << path;
  TextFile file(path, "rt");
  if (!file.IsOpen()) {
    TextFile wf(path, "wt");
    return{};
  }

  std::set<FidoAddress> subscribers;
  std::string line;
  while (file.ReadLine(&line)) {
    StringTrim(&line);
    if (line.empty()) {
      continue;
    }
    try {
      subscribers.insert(FidoAddress(line));
    } catch (const bad_fidonet_address& e) {
      LOG(ERROR) << "ReadFidoSubcriberFile: [" << path.string() << "] : " <<  e.what();
    }
  }
  return subscribers;
}

std::set<uint16_t> ReadSubcriberFile(const std::filesystem::path& path) {
  VLOG(1) << "ReadSubcriberFile: " << path;

  TextFile file(path, "rt");
  if (!file.IsOpen()) {
    TextFile wf(path, "wt");
    return {};
  }

  std::string line;
  std::set<uint16_t> subscribers;
  while (file.ReadLine(&line)) {
    StringTrim(&line);
    if (auto s = to_number<uint16_t>(line); s > 0) {
      subscribers.insert(s);
    }
  }
  return subscribers;
}

bool WriteSubcriberFile(const std::filesystem::path& path, const std::set<uint16_t>& subscribers) {
  TextFile file(path, "wt");
  if (!file.IsOpen()) {
    return false;
  }

  for (const auto s : subscribers) {
    file.WriteLine(std::to_string(s));
  }
  return true;
}

bool WriteFidoSubcriberFile(const std::filesystem::path& path, const std::set<FidoAddress>& subscribers) {
  TextFile file(path, "wt");
  if (!file.IsOpen()) {
    return false;
  }

  for (const auto& s : subscribers) {
    file.WriteLine(s.as_string());
  }
  return true;
}


}

