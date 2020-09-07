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
/*                                                                        */
/**************************************************************************/
#include "common/workspace.h"

#include <memory>
#include <string>
#include "core/file.h"
#include "core/textfile.h"
#include "common/context.h"
#include "common/output.h"
#include "local_io/keycodes.h"
#include "sdk/filenames.h"

namespace wwiv::common {

bool use_workspace;

using std::unique_ptr;
using namespace wwiv::core;

void LoadFileIntoWorkspace(wwiv::common::Context& context, const std::filesystem::path& filename,
                           bool no_edit_allowed, bool silent_mode) {
  TextFile tf(filename, "rt");
  if (!tf) {
    return;
  }
  auto contents = tf.ReadFileIntoString();
  if (contents.empty()) {
    return;
  }

  TextFile input_msg(FilePath(context.session_context().dirs().temp_directory(), INPUT_MSG), "wt");
  input_msg.Write(contents);

  bool ok_fsed = context.u().GetDefaultEditor() != 0;
  use_workspace = (no_edit_allowed || !ok_fsed);

  if (!silent_mode) {
    bout << "\r\nFile loaded into workspace.\r\n\n";
    if (!use_workspace) {
      bout << "Editing will be allowed.\r\n";
    }
  }
}

} // namespace wwiv::comon
