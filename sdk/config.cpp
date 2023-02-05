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
#include "sdk/config.h"

#include "core/datetime.h"
#include "core/file.h"
#include "core/jsonfile.h"
#include "core/stl.h"
#include "core/strings.h"
#include "core/version.h"
#include "sdk/config430.h"
#include "sdk/filenames.h"
#include "sdk/vardec.h"

#include <memory>
#include <string>
#include <utility>

// ReSharper disable once CppUnusedIncludeDirective
#include "sdk/config_cereal.h"

using namespace wwiv::core;
using namespace wwiv::strings;

namespace wwiv::sdk {

Config::Config(std::filesystem::path root_directory, config_t config)
    : initialized_(true), root_directory_(std::move(root_directory)),
      config_(std::move(config)) {
  update_paths();
}

Config::Config(const Config& c) : Config(c.root_directory(), c.config_) {
  // Chained constructor sets initialized to true, but we want to
  // match it from the copy.
  initialized_ = c.IsInitialized();
}

Config::~Config() = default;

Config::Config(std::filesystem::path root_directory) : root_directory_(std::move(root_directory)) {
  initialized_ = Load();
  if (!initialized_) {
    // LOG(ERROR) << config_filename() << " NOT FOUND.";
  }
}

Config::Config(Config&& c) : Config(c.root_directory(), c.config_) {
  // Chained constructor sets initialized to true, but we want to
  // match it from the copy.
  initialized_ = c.IsInitialized();
}


Config& Config::operator=(const Config& o) {
  initialized_ = true;
  root_directory_ = o.root_directory_;
  config_ = o.config_;
  update_paths();
  return *this;
}

Config& Config::operator=(Config&& o) {
  // Our move is really just a copy
  initialized_ = true;
  root_directory_ = o.root_directory_;
  config_ = o.config_;
  update_paths();
  return *this;
}

bool Config::Load() {
  {
    JsonFile f(FilePath(root_directory_, "config.json"), "config", config_, 1);
    if (!f.Load()) {
      return false;
    }
    // We've initialized something. Update absolute paths.
    update_paths();
    versioned_config_dat_ = true;
    readonly_ = false;
  }

  {
    JsonFile f(FilePath(datadir_, "sl.json"), "sl", config_.sl, 1);
    if (!f.Load()) {
      return false;
    }
  }
  {
    JsonFile f(FilePath(datadir_, "autoval.json"), "autoval", config_.autoval, 1);
    if (!f.Load()) {
      return false;
    }
  }

  return true;
}

bool Config::Save() {
  // Update version/date on writes of config.json.
  config_.header.last_written_date = DateTime::now().to_string();
  config_.header.written_by_wwiv_version = full_version();
  config_.header.written_by_wwiv_num_version = wwiv_config_version();

  JsonFile f(FilePath(root_directory_, "config.json"), "config", config_, 1);
  if (readonly_) {
    LOG(ERROR) << "Tried to save a readonly config.json!";
    return false;
  }
  if (!f.Save()) {
    return false;
  }

  JsonFile sl_file(FilePath(datadir_, "sl.json"), "sl", config_.sl, 1);
  sl_file.Save();
  JsonFile av_file(FilePath(datadir_, "autoval.json"), "autoval", config_.autoval, 1);
  av_file.Save();
  return true;
}

void Config::set_config(const config_t& config, bool need_to_update_paths) {
  config_ = config;

  // Update absolute paths.
  if (need_to_update_paths) {
    update_paths();
  }
}

std::filesystem::path Config::scratch_dir(int node) const {
  auto scratch_directory = scratch_format();
  StringReplace(&scratch_directory, "%n", std::to_string(node));
  scratch_directory = File::FixPathSeparators(scratch_directory);
  return FilePath(root_directory(), scratch_directory);
}

void Config::system_name(const std::string& d) { config_.systemname = d; }

void Config::sysop_name(const std::string& d) { config_.sysopname = d; }

void Config::system_phone(const std::string& d) { config_.systemphone = d; }

void Config::system_password(const std::string& d) { config_.systempw = d; }

std::filesystem::path Config::config_filename() const {
  return FilePath(root_directory(), CONFIG_JSON).string();
}

void Config::update_paths() {
  datadir_ = to_abs_path(config_.datadir);
  msgsdir_ = to_abs_path(config_.msgsdir);
  gfilesdir_ = to_abs_path(config_.gfilesdir);
  menudir_ = to_abs_path(config_.menudir);
  dloadsdir_ = to_abs_path(config_.dloadsdir);
  if (config_.scriptdir.empty()) {
    config_.scriptdir = config_.datadir;
  }
  script_dir_ = to_abs_path(config_.scriptdir);
  if (config_.logdir[0]) {
    // If the logdir is empty, leave log_dir_ empty.
    log_dir_ = to_abs_path(config_.logdir);
  }
}

const config_t& Config::to_config_t() const {
  return config_;
}

void Config::set_paths_for_test(const std::filesystem::path& datadir, const std::filesystem::path& msgsdir,
                                const std::filesystem::path& gfilesdir, const std::filesystem::path& menudir,
                                const std::filesystem::path& dloadsdir, const std::filesystem::path& scriptdir) {
  datadir_ = datadir;
  config_.datadir = datadir.string();
  msgsdir_ = msgsdir;
  config_.msgsdir = msgsdir.string();
  gfilesdir_ = gfilesdir;
  config_.gfilesdir = gfilesdir.string();
  menudir_ = menudir;
  config_.menudir = menudir.string();
  dloadsdir_ = dloadsdir;
  config_.dloadsdir = dloadsdir.string();
  script_dir_ = scriptdir;
  config_.scriptdir = scriptdir.string();
}

static void set_script_flag(uint16_t& data, uint16_t flg, bool on) {
  if (on) {
    data |= flg;
  } else {
    data &= ~flg;
  }
}

void Config::newuser_password(const std::string& s) { config_.newuserpw = s; }

void Config::newuser_restrict(uint16_t d) { config_.newuser_restrict = d; }

validation_config_t Config::auto_val(int n) const {
  if (!stl::contains(config_.autoval, n)) {
    return {};
  }
  return stl::at(config_.autoval, n);
}

static const slrec empty_sl{};

const slrec& Config::sl(int n) const {
  if (!stl::contains(config_.sl, n)) {
    return empty_sl;
  }
  return stl::at(config_.sl, n);
}

void Config::sl(int n, slrec& s) { config_.sl[n] = s; }

bool Config::scripting_enabled() const noexcept {
  return !(config_.script_flags & script_flag_disable_script);
}

void Config::scripting_enabled(bool b) noexcept {
  set_script_flag(config_.script_flags, script_flag_disable_script, !b);
}

bool Config::script_package_file_enabled() const noexcept {
  return config_.script_flags & script_flag_enable_file;
}

void Config::script_package_file_enabled(bool b) noexcept {
  set_script_flag(config_.script_flags, script_flag_enable_file, b);
}

bool Config::script_package_os_enabled() const noexcept {
  return config_.script_flags & script_flag_enable_os;
}

void Config::script_package_os_enabled(bool b) noexcept {
  set_script_flag(config_.script_flags, script_flag_enable_os, b);
}

std::filesystem::path Config::to_abs_path(const std::string& dir) const {
  return File::absolute(root_directory_, dir).string();
}

std::filesystem::path LogDirFromConfig(const std::filesystem::path& bbsdir) {
  const Config config{bbsdir};
  if (!config.IsInitialized()) {
    return {};
  }
  return config.logdir();
}

std::unique_ptr<Config> load_any_config(const std::filesystem::path& bbsdir) {
  auto config = std::make_unique<Config>(bbsdir);
  if (config->IsInitialized()) {
    return config;
  }
  // We didn't load config.json, let's try to load config.dat.
  const Config430 c430(bbsdir);
  if (!c430.IsInitialized()) {
    // We couldn't load either.
    LOG(ERROR) << "Unable to load config.json or config.dat";
    return {};
  }
  // We have a good 430.
  LOG(INFO) << "No config.json found, using WWIV 4.x config.dat.";
  return std::make_unique<Config>(bbsdir, c430.to_json_config());
}

} // namespace wwiv::sdk
