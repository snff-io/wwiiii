/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
/*             Copyright (C)2014-2022, WWIV Software Services             */
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
#include "sdk/names.h"

#include "core/datafile.h"
#include "core/file.h"
#include "core/log.h"
#include "core/strings.h"
#include "fmt/format.h"
#include "sdk/config.h"
#include "sdk/filenames.h"
#include "sdk/usermanager.h"
#include "sdk/vardec.h"
#include <algorithm>
#include <string>

using namespace wwiv::core;
using namespace wwiv::strings;

namespace wwiv::sdk {

Names::Names(const wwiv::sdk::Config& config) : data_directory_(config.datadir()) {
  loaded_ = Load();
}

static smalrec smalrec_for(uint32_t user_number, const std::vector<smalrec>& names) {
  for (const auto& n : names) {
    if (n.number == user_number) {
      return n;
    }
  }
  return smalrec{"", 0};
}

std::string Names::UserName(uint32_t user_number) const {
  const auto sr = smalrec_for(user_number, names_);
  if (sr.number == 0) {
    return "";
  }
  const auto name = properize(std::string(reinterpret_cast<const char*>(sr.name)));
  return fmt::format("{} #{}", name, user_number);
}

std::string Names::UserName(uint32_t user_number, uint32_t system_number) const {
  const auto base = UserName(user_number);
  if (base.empty()) {
    return "";
  }
  return fmt::format("{} @{}", base, system_number);
}

bool Names::Add(const std::string& name, uint32_t user_number) {
  const auto upper_case_name = ToStringUpperCase(name);
  auto it = names_.begin();
  for (; it != names_.end()
         && StringCompare(upper_case_name.c_str(), reinterpret_cast<char*>((*it).name)) > 0;
         ++it) {
  }
  smalrec sr{};
  strcpy(reinterpret_cast<char*>(sr.name), upper_case_name.c_str());
  sr.number = static_cast<uint16_t>(user_number);
  names_.insert(it, sr);
  return true;
}

bool Names::AddUnsorted(const std::string& name, uint32_t user_number) {
  const auto upper_case_name = ToStringUpperCase(name);
  smalrec sr{};
  strcpy(reinterpret_cast<char*>(sr.name), upper_case_name.c_str());
  sr.number = static_cast<uint16_t>(user_number);
  names_.emplace_back(sr);
  return true;
}

bool Names::Remove(uint32_t user_number) {
  const std::string name(reinterpret_cast<char*>(smalrec_for(user_number, names_).name));
  if (name.empty()) {
    return false;
  }

  const auto upper_case_name = ToStringUpperCase(name);
  auto it = names_.cbegin();
  for (; it != names_.cend()
         && StringCompare(upper_case_name.c_str(), reinterpret_cast<const char*>((*it).name)) > 0;
         ++it) {
  }
  if (!IsEquals(upper_case_name.c_str(), reinterpret_cast<const char*>((*it).name))) {
    return false;
  }
  names_.erase(it);
  return true;
}

bool Names::Load() {
  DataFile<smalrec> file(FilePath(data_directory_, NAMES_LST));
  if (!file) {
    return false;
  }
  names_.clear();
  return file.ReadVector(names_);
}

bool Names::Save() {
  DataFile<smalrec> file(FilePath(data_directory_, NAMES_LST),
                         File::modeReadWrite | File::modeBinary | File::modeTruncate |
                             File::modeCreateFile);
  if (!file) {
    LOG(ERROR) << "Error saving NAMES.LST";
    return false;
  }

  std::sort(names_.begin(), names_.end(), [](const smalrec& a, const smalrec& b) -> bool {
    const auto equal = strcmp((char*)a.name, (char*)b.name);
    // Sort by user number if names match.
    if (equal == 0) {
      return a.number < b.number;
    }
    // Otherwise sort by name comparison.
    return equal < 0;
  });

  return file.WriteVector(names_);
}

bool Names::Rebuild(const UserManager& um) {
  const auto num_user_records = um.num_user_records();
  if (num_user_records < 1) {
    return false;
  }

  names_.clear();
  for (auto i = 1; i <= num_user_records; i++) {
    if (const auto user = um.readuser(i, UserManager::mask::active)) {
      AddUnsorted(user->name(), i);
    }
  }
  return true;
}


int Names::FindUser(const std::string& search_string) {
  for (const auto& n : names_) {
    if (iequals(search_string.c_str(), reinterpret_cast<const char*>(n.name))) {
      return n.number;
    }
  }
  return 0;
}

Names::~Names() {
  if (!save_on_exit_) {
    return;
  }
  Save();
}

}
