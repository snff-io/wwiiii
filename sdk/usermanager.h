/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*             Copyright (C)1998-2022, WWIV Software Services            */
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

#ifndef INCLUDED_USER_MANAGER_H
#define INCLUDED_USER_MANAGER_H

#include "sdk/config.h"
#include "sdk/user.h"
#include <filesystem>
#include <string>

namespace wwiv::sdk {

/**
 * WWIV User Manager.
 * 
 * Responsible for loading and saving users.
 */
class UserManager {
public:
  // active means non_deleted and non_inactive
  enum class mask { any, non_deleted, non_inactive, active };

  UserManager() = delete;
   explicit UserManager(const Config& config);
   virtual ~UserManager();
   [[nodiscard]] int num_user_records() const;
   bool readuser(User *user, int user_number) const;

  /**
    * Optionally returns the user specified by user_number.
    */
   [[nodiscard]] std::optional<User> readuser(int user_number, mask m = mask::any) const;

   bool writeuser(const User *pUser, int user_number);
   bool writeuser(const User &user, int user_number);
   bool writeuser(const std::optional<User>& user, int user_number);

   bool delete_user(int user_number);
   bool restore_user(int user_number);

  /**
   * Setting this to false will disable writing the userrecord to disk.  This should ONLY be false when the
   * user is the guest user.
   */
  void set_user_writes_allowed(bool a) {
    allow_writes_ = a;
  }

   [[nodiscard]] bool user_writes_allowed() const {
    return allow_writes_;
  }

private:
  const Config config_;
  const std::filesystem::path data_directory_;
  int userrec_length_;
  int max_number_users_;
  bool allow_writes_{false};
};

}  // namespace

#endif
