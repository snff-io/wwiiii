/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
/*             Copyright (C)1998-2020, WWIV Software Services             */
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
#ifndef __INCLUDED_BBS_INTERPRET_H__
#define __INCLUDED_BBS_INTERPRET_H__

#include "common/context.h"
#include "sdk/ansi/ansi.h"
#include "sdk/files/dirs.h"
#include "sdk/user.h"
#include <string>


class MacroContext {
public:
  MacroContext(wwiv::common::Context* context) : context_(context) {}
  ~MacroContext() = default;

  std::string interpret(char c) const;

private:
  wwiv::common::Context* context_{nullptr};
};

class BbsMacroFilter : public wwiv::sdk::ansi::AnsiFilter {
public:
  BbsMacroFilter(wwiv::sdk::ansi::AnsiFilter* chain, const MacroContext* ctx)
      : chain_(chain), ctx_(ctx){};
  bool write(char c) override;
  bool attr(uint8_t a) override;

private:
  wwiv::sdk::ansi::AnsiFilter* chain_;
  const MacroContext* ctx_;
  bool in_pipe_{false};
  bool in_macro_{false};
};

#endif // __INCLUDED_BBS_INTERPRET_H__
