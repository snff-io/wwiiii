/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.x                          */
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
#include "bbs/misccmd.h"

#include "bbs/acs.h"
#include "bbs/bbs.h"
#include "bbs/confutil.h"
#include "bbs/email.h"
#include "bbs/msgbase1.h"
#include "bbs/read_message.h"
#include "bbs/sysoplog.h"
#include "bbs/utility.h"
#include "bbs/wqscn.h"
#include "common/com.h"
#include "common/datetime.h"
#include "common/input.h"
#include "core/strings.h"
#include "fmt/printf.h"
#include "local_io/keycodes.h"
#include "local_io/wconstants.h"
#include "sdk/filenames.h"
#include "sdk/names.h"
#include "sdk/status.h"
#include "sdk/user.h"
#include "sdk/usermanager.h"
#include "sdk/files/dirs.h"
#include "sdk/net/networks.h"

#include <memory>
#include <string>

using namespace wwiv::core;
using namespace wwiv::local::io;
using namespace wwiv::sdk;
using namespace wwiv::sdk::net;
using namespace wwiv::strings;

void kill_old_email() {
  mailrec m{};
  mailrec m1{};
  User user;
  filestatusrec fsr{};

  bout.outstr("|#5List mail starting at most recent? ");
  bool forward = bin.yesno();
  auto pFileEmail(OpenEmailFile(false));
  if (!pFileEmail->IsOpen()) {
    bout.outstr("\r\nNo mail.\r\n");
    return;
  }
  auto max = static_cast<int>(pFileEmail->length() / sizeof(mailrec));
  int cur = 0;
  if (forward) {
    cur = max - 1;
  }

  bool done = false;
  do {
    pFileEmail->Seek(cur * sizeof(mailrec), File::Whence::begin);
    pFileEmail->Read(&m, sizeof(mailrec));
    while ((m.fromsys != 0 || m.fromuser != a()->sess().user_num() || m.touser == 0) && cur < max && cur >= 0) {
      if (forward) {
        --cur;
      } else {
        ++cur;
      }
      if (cur < max && cur >= 0) {
        pFileEmail->Seek(cur * sizeof(mailrec), File::Whence::begin);
        pFileEmail->Read(&m, sizeof(mailrec));
      }
    }
    if (m.fromsys != 0 || m.fromuser != a()->sess().user_num() || m.touser == 0 || cur >= max || cur < 0) {
      done = true;
    } else {
      pFileEmail->Close();

      bool done1 = false;
      do {
        bout.nl();
        bout.outstr("|#1  To|#9: ");
        bout.ansic(a()->GetMessageColor());

        if (m.tosys == 0) {
          a()->users()->readuser(&user, m.touser);
          std::string tempName = a()->user()->name_and_number();
          if ((m.anony & (anony_receiver | anony_receiver_pp | anony_receiver_da))
              && ((a()->config()->sl(a()->sess().effective_sl()).ability & ability_read_email_anony) == 0)) {
            tempName = ">UNKNOWN<";
          }
          bout.outstr(tempName);
          bout.nl();
        } else {
          bout.print("#{} @{}\r\n", m.tosys, m.tosys);
        }
        bout.printf("|#1Subj|#9: |#%d%60.60s\r\n", a()->GetMessageColor(), m.title);
        time_t lCurrentTime = time(nullptr);
        int nDaysAgo = static_cast<int>((lCurrentTime - m.daten) / SECONDS_PER_DAY);
        bout.outstr("|#1Sent|#9: ");
        bout.ansic(a()->GetMessageColor());
        bout.print("{} days ago\r\n", nDaysAgo);
        if (m.status & status_file) {
          File fileAttach(FilePath(a()->config()->datadir(), ATTACH_DAT));
          if (fileAttach.Open(File::modeBinary | File::modeReadOnly)) {
            bool found = false;
            auto l1 = fileAttach.Read(&fsr, sizeof(fsr));
            while (l1 > 0 && !found) {
              if (m.daten == static_cast<uint32_t>(fsr.id)) {
                bout.print("|#1Filename|#0.... |#2{} ({} bytes)|#0\r\n", fsr.filename,
                           fsr.numbytes);
                found = true;
              }
              if (!found) {
                l1 = fileAttach.Read(&fsr, sizeof(fsr));
              }
            }
            if (!found) {
              bout.outstr("|#1Filename|#0.... |#2Unknown or missing|#0\r\n");
            }
            fileAttach.Close();
          } else {
            bout.outstr("|#1Filename|#0.... |#2Unknown or missing|#0\r\n");
          }
        }
        bout.nl();
        bout.outstr("|#9(R)ead, (D)elete, (N)ext, (Q)uit : ");
        switch (char ch = onek("QRDN"); ch) {
        case 'Q':
          done1   = true;
          done    = true;
          break;
        case 'N':
          done1 = true;
          if (forward) {
            --cur;
          } else {
            ++cur;
          }
          if (cur >= max || cur < 0) {
            done = true;
          }
          break;
        case 'D': {
          done1 = true;
          auto delete_email_file = OpenEmailFile(true);
          delete_email_file->Seek(cur * sizeof(mailrec), File::Whence::begin);
          delete_email_file->Read(&m1, sizeof(mailrec));
          if (memcmp(&m, &m1, sizeof(mailrec)) == 0) {
            delmail(*delete_email_file, cur);
            bool found = false;
            if (m.status & status_file) {
              File fileAttach(FilePath(a()->config()->datadir(), ATTACH_DAT));
              if (fileAttach.Open(File::modeBinary | File::modeReadWrite)) {
                auto l1 = fileAttach.Read(&fsr, sizeof(fsr));
                while (l1 > 0 && !found) {
                  if (m.daten == static_cast<uint32_t>(fsr.id)) {
                    found = true;
                    fsr.id = 0;
                    fileAttach.Seek(static_cast<long>(sizeof(filestatusrec)) * -1L, File::Whence::current);
                    fileAttach.Write(&fsr, sizeof(filestatusrec));
                    File::Remove(FilePath(a()->GetAttachmentDirectory(), fsr.filename));
                  } else {
                    l1 = fileAttach.Read(&fsr, sizeof(filestatusrec));
                  }
                }
                fileAttach.Close();
              }
            }
            bout.nl();
            if (found) {
              bout.outstr("Mail and file deleted.\r\n\n");
              sysoplog(fmt::format("Deleted mail and attached file: {}", fsr.filename));
            } else {
              bout.outstr("Mail deleted.\r\n\n");
              const std::string username_num = a()->names()->UserName(m1.touser);
              sysoplog(fmt::format("Deleted mail sent to {}", username_num));
            }
          } else {
            bout.outstr("Mail file changed; try again.\r\n");
          }
          delete_email_file->Close();
        }
        break;
        case 'R': {
          bout.nl(2);
          if (auto o = read_type2_message(&m.msg, m.anony & 0x0f, false, "email", 0, 0)) {
            auto& msg = o.value();
            msg.title = m.title;
            msg.message_area = "Personal E-Mail";
            auto next = false;
            auto fake_msgno = -1;
            display_type2_message(fake_msgno, msg, &next);
          }
        }
        break;
        }
      } while (!a()->sess().hangup() && !done1);
      pFileEmail = OpenEmailFile(false);
      if (!pFileEmail->IsOpen()) {
        break;
      }
    }
  } while (!done && !a()->sess().hangup());
  pFileEmail->Close();
}

void list_users(int mode) {
  subboard_t s = {};
  files::directory_t d{};
  User user;
  std::string find_text;

  if (a()->current_user_sub().subnum == -1 && mode == LIST_USERS_MESSAGE_AREA) {
    bout.outstr("\r\n|#6No Message Sub Available!\r\n\n");
    return;
  }
  if (a()->current_user_dir().subnum == -1 && mode == LIST_USERS_FILE_AREA) {
    bout.outstr("\r\n|#6 No Dirs Available.\r\n\n");
    return;
  }

  auto snum = a()->sess().user_num();

  bout.nl();
  bout.outstr("|#5Sort by user number? ");
  bool bSortByUserNumber = bin.yesno();
  bout.nl();
  bout.outstr("|#5Search for a name or city? ");
  if (bin.yesno()) {
    bout.nl();
    bout.outstr("|#5Enter text to find: ");
    find_text = bin.input_upper(10);
  }

  if (mode == LIST_USERS_MESSAGE_AREA) {
    s = a()->subs().sub(a()->current_user_sub().subnum);
  } else {
    d = a()->dirs()[a()->current_user_dir().subnum];
  }

  bool abort  = false;
  bool next   = false;
  int p       = 0;
  int num     = 0;
  bool found  = true;
  int count   = 0;
  int ncnm    = 0;
  int numscn  = 0;
  int color   = 3;
  a()->WriteCurrentUser();
  write_qscn(a()->sess().user_num(), a()->sess().qsc, false);
  a()->status_manager()->reload_status();

  File userList(FilePath(a()->config()->datadir(), USER_LST));
  int nNumUserRecords = a()->users()->num_user_records();

  for (int i = 0; (i < nNumUserRecords) && !abort && !a()->sess().hangup(); i++) {
    a()->sess().user_num(0);
    if (ncnm > 5) {
      count++;
      bout.ansic(color);
      bout.outstr(".");
      if (count == NUM_DOTS) {
        bout.outstr("\r", &abort, &next);
        bout.outstr("|#2Searching ", &abort, &next);
        color++;
        count = 0;
        if (color == 4) {
          color++;
        }
        if (color == 7) {
          color = 0;
        }
      }
    }
    if (p == 0 && found) {
      bout.cls();
      auto title_line = StrCat(a()->config()->system_name(), " User Listing");
      if (okansi()) {
        bout.litebar(title_line);
      } else {
        int i1;
        for (i1 = 0; i1 < 78; i1++) {
          bout.outchr(45);
        }
        bout.nl();
        bout.print("|#5{}", title_line);
        bout.nl();
        for (i1 = 0; i1 < 78; i1++) {
          bout.outchr(45);
        }
        bout.nl();
      }
      bout.ansic(FRAME_COLOR);
      bout.bpla("\xD5\xCD\xCD\xCD\xCD\xCD\xCD\xD1\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xD1\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xD1\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xD1\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xB8",
          &abort);
      found = false;
    }

    int user_number = (bSortByUserNumber) ? i + 1 : a()->names()->names_vector()[i].number;
    a()->users()->readuser(&user, user_number);
    read_qscn(user_number, a()->sess().qsc, false);
    changedsl();
    bool in_qscan = (a()->sess().qsc_q[a()->current_user_sub().subnum / 32] &
                     (1L << (a()->current_user_sub().subnum % 32)))
                        ? true
                        : false;
    bool ok = true;
    if (user.deleted()) {
      ok = false;
    }
    if (mode == LIST_USERS_MESSAGE_AREA && !wwiv::bbs::check_acs(s.read_acs)) {
        ok = false;
    }
    if (mode == LIST_USERS_FILE_AREA && !wwiv::bbs::check_acs(d.acs)) {
      ok = false;
    }
    if (!find_text.empty()) {
      char s5[ 41 ];
      to_char_array(s5, user.city());
      if (!strstr(user.GetName(), find_text.c_str()) &&
          !strstr(strupr(s5), find_text.c_str()) &&
          !strstr(user.state().c_str(), find_text.c_str())) {
        ok = false;
      }
    }
    if (ok) {
      found = true;
      bout.clreol();
      if (user.last_bps() > 32767 || user.last_bps() < 300) {
        user.last_bps(33600);
      }
      std::string city = "Unknown";
      if (!user.city().empty()) {
        city = fmt::format("{:.18}, {}", user.city(), user.state());
      }
      auto properName = properize(user.name());
      const auto line = fmt::sprintf("|#%d\xB3|#9%5d |#%d\xB3|#6%c|#1%-20.20s|#%d\xB3|#2 "
                                     "%-24.24s|#%d\xB3 |#1%-9s |#%d\xB3  |#3%-5u  |#%d\xB3",
                                     FRAME_COLOR, user_number, FRAME_COLOR, in_qscan ? '*' : ' ',
                                     properName, FRAME_COLOR, city, FRAME_COLOR, user.laston(),
                                     FRAME_COLOR, user.last_bps(), FRAME_COLOR);
      bout.bpla(line, &abort);
      num++;
      if (in_qscan) {
        numscn++;
      }
      ++p;
      if (p == static_cast<int>(a()->user()->screen_lines()) - 6) {
        //bout.backline();
        bout.clreol();
        bout.ansic(FRAME_COLOR);
        bout.bpla("\xD4\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBE",
            &abort);
        bout.outstr("|#1[Enter] to continue or Q=Quit : ");
        switch (auto ch = onek("Q\r "); ch) {
        case 'Q':
          abort = true;
          i = a()->status_manager()->user_count();
          break;
        case SPACE:
        case RETURN:
          p = 0;
          break;
        }
      }
    } else {
      ncnm++;
    }
  }
  //bout.backline();
  bout.clreol();
  bout.ansic(FRAME_COLOR);
  bout.bpla("\xD4\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCF\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xBE",
      &abort);
  if (!abort) {
    bout.nl(2);
    bout.print("|#1{} user(s) have access and {} user(s) scan this subboard.",  num , numscn);
    bout.nl();
    bout.pausescr();
  }
  a()->ReadCurrentUser(snum);
  read_qscn(snum, a()->sess().qsc, false);
  a()->sess().user_num(snum);
  changedsl();
}


void time_bank() {
  char s[81], bc[81];

  bout.nl();
  if (!wwiv::bbs::check_acs("user.validated == true")) {
    bout.outstr("|#6You must be validated to access the timebank.\r\n");
    return;
  }
  if (a()->user()->banktime_minutes() > a()->config()->sl(a()->sess().effective_sl()).time_per_logon) {
    a()->user()->banktime_minutes(a()->config()->sl(a()->sess().effective_sl()).time_per_logon);
  }

  if (okansi()) {
    strcpy(bc, "ڿ��ĳ��Ŵ�");
  } else {
    strcpy(bc, "++++-|+++++");
  }

  bool done = false;
  do {
    bout.cls();
    bout.outstr("|#5WWIV TimeBank\r\n");
    bout.nl();
    bout.outstr("|#2D|#9)eposit Time\r\n");
    bout.outstr("|#2W|#9)ithdraw Time\r\n");
    bout.outstr("|#2Q|#9)uit\r\n");
    bout.nl();
    bout.print("|#9Balance:   |#2{}|#9 minutes\r\n", a()->user()->banktime_minutes());
    bout.print("|#9Time Left: |#2{}|#9 minutes\r\n", static_cast<int>(nsl() / 60));
    bout.nl();
    bout.outstr("|#9(|#2Q|#9=|#1Quit|#9) [|#2Time Bank|#9] Enter Command: |#2");
    bout.mpl(1);
    switch (char c = onek("QDW"); c) {
    case 'D': {
      bout.nl();
      bout.outstr("|#1Deposit how many minutes: ");
      bin.input(s, 3, true);
      auto i = to_number<int>(s);
      if (i > 0) {
        long nsln = nsl();
        if ((i + a()->user()->banktime_minutes()) >
            a()->config()->sl(a()->sess().effective_sl()).time_per_logon) {
          i = a()->config()->sl(a()->sess().effective_sl()).time_per_logon -
              a()->user()->banktime_minutes();
        }
        if (i > (nsln / SECONDS_PER_MINUTE)) {
          i = static_cast<int>(nsln / SECONDS_PER_MINUTE);
        }
        a()->user()->add_banktime_minutes(i);
        a()->user()->subtract_extratime(std::chrono::minutes(i));
        a()->tleft(false);
      }
    } break;
    case 'W': {
      bout.nl();
      if (a()->user()->banktime_minutes() == 0) {
        break;
      }
      bout.outstr("|#1Withdraw How Many Minutes: ");
      bin.input(s, 3, true);
      auto i = to_number<int>(s);
      if (i > 0) {
        if (i > a()->user()->banktime_minutes()) {
          i = a()->user()->banktime_minutes();
        }
        a()->user()->subtract_banktime_minutes(i);
        a()->user()->add_extratime(std::chrono::minutes(i));
        a()->tleft(false);
      }
    } break;
    case 'Q':
      done = true;
      break;
    }
  } while (!done && !a()->sess().hangup());
}

int getnetnum(const std::string& network_name) {
  for (auto i = 0; i < wwiv::stl::size_int(a()->nets()); i++) {
    if (iequals(a()->nets()[i].name, network_name)) {
      return i;
    }
  }
  return -1;
}

int getnetnum_by_type(network_type_t type) {
  const auto& n = a()->nets().networks();
  for (auto i = 0; i < wwiv::stl::ssize(a()->nets()); i++) {
    if (n[i].type == type) {
      return i;
    }
  }
  return -1;
}
