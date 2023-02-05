/**************************************************************************/
/*                                                                        */
/*                          WWIV Version 5.x                              */
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
#ifndef INCLUDED_BINKP_NET_LOG_H
#define INCLUDED_BINKP_NET_LOG_H

#include "sdk/config.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <string>

namespace wwiv::net {

/**
 * Handles writing new log entries into net.log.

 * Format of gfiles/net.log:
 *
 * 01/03/15 20:26:23 To 32767,                             0.1 min  wwivnet
 * 01/03/15 20:26:23 To     1, S : 4k, R : 3k,             0.1 min  wwivnet
 */

enum class NetworkSide { FROM, TO };

class NetworkLog final {
public:
  explicit NetworkLog(const std::filesystem::path& gfiles_directory);
  ~NetworkLog();

  bool Log(time_t time, NetworkSide side, int node, unsigned int bytes_sent,
           unsigned int bytes_received, std::chrono::seconds seconds_elapsed,
           const std::string& network_name);
  [[nodiscard]] std::string GetContents() const;

  [[nodiscard]] std::string ToString() const;

  [[nodiscard]] std::string CreateLogLine(time_t time, NetworkSide side, int node, int bytes_sent,
                                          int bytes_received, std::chrono::seconds seconds_elapsed,
                                          const std::string& network_name) const;

private:
  const std::filesystem::path gfiles_directory_;
};

} // namespace wwiv::net

#endif