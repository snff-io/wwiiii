/**************************************************************************/
/*                                                                        */
/*                  WWIV Initialization Utility Version 5                 */
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
/*                                                                        */
/**************************************************************************/
#include "wwivconfig/subacc.h"

#include "core/file.h"
#include "core/stl.h"
#include "core/strings.h"
#include "fmt/format.h"
#include "sdk/config.h"
#include "sdk/subxtr.h"
#include "sdk/vardec.h"
#include <memory>
#include <string>

using namespace wwiv::core;
using namespace wwiv::stl;
using namespace wwiv::strings;

// File object for '.sub' file
static std::unique_ptr<File> fileSub;             
// filename of .sub file. Ideally this shoudn't be a string but we don't use it
// from joined threads nor from other initializers executed before the program starts
static std::filesystem::path subdat_fn; 

// locals
static int current_read_message_area, subchg;
static int GetCurrentReadMessageArea() { return current_read_message_area; }

static void SetCurrentReadMessageArea(int n) { current_read_message_area = n; }

static int nNumMsgsInCurrentSub;

int GetNumMessagesInCurrentMessageArea() { return nNumMsgsInCurrentSub; }

static void SetNumMessagesInCurrentMessageArea(int n) { nNumMsgsInCurrentSub = n; }

void close_sub() {
  if (fileSub) {
    fileSub.reset();
  }
}

bool open_sub(bool wr) {
  postrec p{};

  close_sub();

  if (wr) {
    fileSub.reset(new File(subdat_fn));
    fileSub->Open(File::modeBinary | File::modeCreateFile | File::modeReadWrite);

    if (fileSub->IsOpen()) {
      // re-read info from file, to be safe
      fileSub->Seek(0L, File::Whence::begin);
      fileSub->Read(&p, sizeof(postrec));
      SetNumMessagesInCurrentMessageArea(p.owneruser);
    }
  } else {
    fileSub.reset(new File(subdat_fn));
    fileSub->Open(File::modeReadOnly | File::modeBinary);
  }

  return fileSub->IsOpen();
}

bool iscan1(int si, const wwiv::sdk::Subs& subs, const wwiv::sdk::Config& config) {
  // Initializes use of a sub value (subs[], not a()->usub[]).  If quick, then
  // don't worry about anything detailed, just grab qscan info.
  postrec p{};

  // forget it if an invalid sub #
  if (si < 0 || si >= size_int(subs.subs())) {
    return false;
  }

  // see if a sub has changed
  if (subchg) {
    SetCurrentReadMessageArea(-1);
  }

  // if already have this one set, nothing more to do
  if (si == GetCurrentReadMessageArea()) {
    return true;
  }

  // set sub filename
  subdat_fn = FilePath(config.datadir(), fmt::format("{}.sub", subs.sub(si).filename));

  // open file, and create it if necessary
  if (!File::Exists(subdat_fn)) {
    if (!open_sub(true)) {
      return false;
    }
    p.owneruser = 0;
    fileSub->Write(&p, sizeof(postrec));
  } else if (!open_sub(false)) {
    return false;
  }

  // set sub
  SetCurrentReadMessageArea(si);
  subchg = 0;

  // read in first rec, specifying # posts
  fileSub->Seek(0L, File::Whence::begin);
  fileSub->Read(&p, sizeof(postrec));
  SetNumMessagesInCurrentMessageArea(p.owneruser);

  // close file
  close_sub();

  // iscanned correctly
  return true;
}

std::optional<postrec> get_post(int mn) {
  // Returns info for a post.  Does not correct anything if the sub has changed.
  // returns null optional if msg # invalid
  if (mn < 1) {
    return std::nullopt;
  }
  // adjust msgnum, if it is no longer valid
  if (mn > GetNumMessagesInCurrentMessageArea()) {
    mn = GetNumMessagesInCurrentMessageArea();
  }

  // read in some sub info
  postrec p{};
  fileSub->Seek(mn * sizeof(postrec), File::Whence::begin);
  fileSub->Read(&p, sizeof(postrec));
  return {p};
}

void write_post(int mn, const postrec& pp) {
  if (fileSub->IsOpen()) {
    fileSub->Seek(mn * sizeof(postrec), File::Whence::begin);
    fileSub->Write(&pp, sizeof(postrec));
  }
}
