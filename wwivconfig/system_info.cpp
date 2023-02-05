/**************************************************************************/
/*                                                                        */
/*                  WWIV Initialization Utility Version 5                 */
/*               Copyright (C)2014-2022, WWIV Software Services           */
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
/*                                                                        */
/**************************************************************************/
#include "wwivconfig/system_info.h"

#include "core/strings.h"
#include "fmt/printf.h"
#include "localui/input.h"
#include "localui/curses_win.h"
#include "localui/edit_items.h"
#include "sdk/vardec.h"
#include "wwivconfig/new_user.h"
#include "wwivconfig/colors.h"
#include "wwivconfig/toggles.h"
#include "wwivconfig/utility.h"
#include <cstdint>
#include <string>

using namespace wwiv::local::ui;
using namespace wwiv::strings;

static std::string print_time(uint16_t t) {
  return fmt::sprintf("%02d:%02d", t / 60, t % 60);
}

static uint16_t get_time(const std::string& s) {
  if (s[2] != ':') {
    return std::numeric_limits<uint16_t>::max();
  }

  const auto h = to_number<uint16_t>(s);
  const auto minutes = s.substr(3);
  const auto m = to_number<uint16_t>(minutes);
  if (h > 23 || m > 59) {
    return std::numeric_limits<uint16_t>::max();
  }
  return static_cast<uint16_t>(h * 60 + m);
}

static const int MAX_TIME_EDIT_LEN = 5;

class TimeEditItem final : public EditItem<uint16_t*> {
public:
  TimeEditItem(uint16_t* data) : EditItem<uint16_t*>(5, data) {}
  ~TimeEditItem() override = default;

  EditlineResult Run(CursesWindow* window) override {
    window->GotoXY(this->x_, this->y_);
    auto s = print_time(*this->data_);
    const auto return_code = editline(window, &s, MAX_TIME_EDIT_LEN + 1, EditLineMode::ALL, "");
    *this->data_ = get_time(s);
    return return_code;
  }

protected:
  void DefaultDisplay(CursesWindow* window) const override {
    const auto s = print_time(*this->data_);
    DefaultDisplayString(window, s);
  }
};

class Float53EditItem final : public EditItem<float*> {
public:
  Float53EditItem(float* data) : EditItem<float*>(5, data) {}
  ~Float53EditItem() override = default;

  EditlineResult Run(CursesWindow* window) override {
    window->GotoXY(this->x_, this->y_);
   
    // passing *this->data_ to String Printf is causing a bus error
    // on GCC/ARM (RPI).  See http://stackoverflow.com/questions/26158510
    const auto d = *this->data_;
    auto s = fmt::sprintf("%5.3f", d);
    const auto return_code = editline(window, &s, 5 + 1, EditLineMode::NUM_ONLY, "");

    char* e;
    auto f = strtof(s.c_str(), &e);
    if (f > 9.999 || f < 0.001) {
      f = 0.0;
    }
    *this->data_ = f;
    return return_code;
  }

protected:
  void DefaultDisplay(CursesWindow* window) const override {
    // passing *this->data_ to fmt::sprintf is causing a bus error
    // on GCC/ARM (RPI).  See http://stackoverflow.com/questions/26158510
    const auto d = *this->data_;
    const auto s = fmt::sprintf("%5.3f", d);
    DefaultDisplayString(window, s);
  }
};


template <class T> class NewUserSubDialog final : public SubDialog<T> {
public:
  NewUserSubDialog(wwiv::sdk::Config& c, T& t,
                   std::function<void(wwiv::sdk::Config&, T&, CursesWindow*)> fn)
      : SubDialog<T>(c, t), c_(c), t__(t), fn_(std::move(fn)) {}
  ~NewUserSubDialog() override = default;

  void RunSubDialog(CursesWindow* window) override { fn_(c_, t__, window); }

private:
  // For some reason GCC couldn't find config() or t_ from SubDialog.
  wwiv::sdk::Config& c_;
  T& t__;
  std::function<void(wwiv::sdk::Config&, T&, CursesWindow*)> fn_;
};


void sysinfo1(wwiv::sdk::Config& config) {
  statusrec_t statusrec{};
  read_status(config.datadir(), statusrec);

  if (statusrec.callernum != 65535) {
    statusrec.callernum1 = static_cast<long>(statusrec.callernum);
    statusrec.callernum = 65535;
    save_status(config.datadir(), statusrec);
  }

  auto system_name = config.system_name();
  auto sysop_name = config.sysop_name();
  auto system_phone = config.system_phone();
  auto system_pw = config.system_password();
  auto sysoplowtime = config.sysop_low_time();
  auto sysophightime = config.sysop_high_time();
  auto req_ratio = config.req_ratio();
  auto post_call_ratio = config.post_to_call_ratio();
  auto max_users = config.max_users();
  int max_waiting = config.max_waiting();
  int wwiv_reg_number = config.wwiv_reg_number();
  int max_backups = config.max_backups();
  int num_instances = config.num_instances();

  auto y = 1;
  EditItems items{};
  items.add(new Label("System name:"),
            new StringEditItem<std::string&>(50, system_name, EditLineMode::ALL),
    "Name of the BBS System", 1, y);
  ++y;
  items.add(new Label("Sysop name:"),
            new StringEditItem<std::string&>(50, sysop_name, EditLineMode::ALL),
    "The name (or handle/alias) of the System Operator", 1, y);
  ++y;
  items.add(
      new Label("System phone:"),
      new StringEditItem<std::string&>(12, system_phone, EditLineMode::UPPER_ONLY),
    "Phone number for the BBS (if you have one)", 1, y);
  ++y;
  items.add(new Label("System PW:"),
            new StringEditItem<std::string&>(20, system_pw, EditLineMode::UPPER_ONLY),
            "Password needed to log in as sysop", 1, y);
  y+=2;
  items.add(new Label("Sysop time: from:"),
            new TimeEditItem(&sysoplowtime),
    "Set the time limits that the sysop is available for chat", 1, y);
  items.add(new Label("to:"),
            new TimeEditItem(&sysophightime),
    "Set the time limits that the sysop is available for chat", 3, y);
  ++y;
  items.add(new Label("Ratios    :  U/D:"),
            new Float53EditItem(&req_ratio),
    "Optional required ratio of (uploads/downloads) for downloading files", 1, y);
  items.add(new Label("Post/Call:"),
            new Float53EditItem(&post_call_ratio),
    "Optional required ratio of (uploads/downloads) for downloading files", 3, y);
  ++y;
  items.add(new Label("Max waiting:"),
            new NumberEditItem<int, 3>(&max_waiting),
    "Maximum number of emails allowed for a user", 1, y);
  items.add(new Label("Max users:"),
            new NumberEditItem<uint16_t>(&max_users),
    "The maximum number of users that can be on the system", 3, y);
  ++y;
  items.add(new Label("Total Calls:"),
            new NumberEditItem<uint32_t>(&statusrec.callernum1),
    "Caller number for the last call to the BBS.", 1, y);
  items.add(new Label("Days active:"),
            new NumberEditItem<uint16_t>(&statusrec.days),
    "Number of days the BBS has been active", 3, y);
  ++y;
  items.add(new Label("4.x Reg Number:"),
            new NumberEditItem<int>(&wwiv_reg_number),
    "Legacy registration # from WWIV 4.xx. Just used for bragging rights", 1, y);

  items.add(new Label("Max Backups:"),
            new NumberEditItem<int, 3>(&max_backups),
    "Max number of backup to keep when making new datafile backups (0=unlimited)", 3, y);
  ++y;
  items.add(new Label("Max Instances:"),
            new NumberEditItem<int, 3>(&num_instances),
    "Max number of BBS instances allowed.", 1, y);

  y += 2;
  items.add(new Label("Newuser Settings:"),
            new NewUserSubDialog<wwiv::sdk::newuser_config_t>(config, config.newuser_config(), newuser_settings),
            1, y);
  ++y;
  items.add(new Label("System Toggles:"),
            new NewUserSubDialog<wwiv::sdk::system_toggles_t>(config, config.toggles(), toggles), 1,
            y);
  ++y;
  items.add(new Label("System Colors:"),
            new NewUserSubDialog<wwiv::sdk::color_config_t>(config, config.colors(), colors), 1,
            y);
  ++y;

  items.add_aligned_width_column(1);
  items.relayout_items_and_labels();
  items.Run("System Configuration");

  config.system_name(system_name);
  config.sysop_name(sysop_name);
  config.system_phone(system_phone);
  config.system_password(system_pw);
  config.sysop_low_time(sysoplowtime);
  config.sysop_high_time(sysophightime);
  config.req_ratio(req_ratio);
  config.post_to_call_ratio(post_call_ratio);
  config.max_users(max_users);
  config.max_waiting(max_waiting);
  config.wwiv_reg_number(wwiv_reg_number);
  config.max_backups(max_backups);
  config.num_instances(num_instances);

  save_status(config.datadir(), statusrec);
}
