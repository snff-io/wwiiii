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
#include "bbs/exec.h"

// Always declare wwiv_windows.h first to avoid collisions on defines.
#include "core/wwiv_windows.h"

#include "bbs/application.h"
#include "bbs/bbs.h"
#include "bbs/dropfile.h"
#include "bbs/exec_socket.h"
#include "bbs/shortmsg.h"
#include "bbs/sysoplog.h"
#include "common/output.h"
#include "common/remote_io.h"
#include "core/file.h"
#include "core/log.h"
#include "core/net.h"
#include "core/scope_exit.h"
#include "core/strings.h"
#include "core/textfile.h"
#include "fmt/format.h"
#include <algorithm>
#include <atomic>
#include <sstream>
#include <string>

#ifdef _WIN32
#define NOCRYPT // Disable include of wincrypt.h
#include <winsock2.h>
#endif // _WIN32

static FILE* hLogFile;

const int CONST_SBBSFOS_FOSSIL_MODE = 0;
const int CONST_SBBSFOS_DOSIN_MODE = 1;
const int CONST_SBBSFOS_DOSOUT_MODE = 2;
const int CONST_SBBSFOS_LOOPS_BEFORE_YIELD = 10;
const int CONST_SBBSFOS_BUFFER_SIZE = 5000;
const int CONST_SBBSFOS_BUFFER_PADDING = 1000; // padding in case of errors.

const DWORD SBBSEXEC_IOCTL_START = 0x8001;
const DWORD SBBSEXEC_IOCTL_COMPLETE = 0x8002;
const DWORD SBBSEXEC_IOCTL_READ = 0x8003;
const DWORD SBBSEXEC_IOCTL_WRITE = 0x8004;
const DWORD SBBSEXEC_IOCTL_DISCONNECT = 0x8005;
const DWORD SBBSEXEC_IOCTL_STOP = 0x8006;
const int CONST_NUM_LOOPS_BEFORE_EXIT_CHECK = 500;
typedef HANDLE(WINAPI* OPENVXDHANDLEFUNC)(HANDLE);

using namespace std::chrono_literals;
using namespace wwiv::bbs;
using namespace wwiv::core;
using namespace wwiv::strings;

// Helper functions

static void LogToSync(const std::string& s) {
  if (a()->IsExecLogSyncFoss()) {
    fputs(s.c_str(), hLogFile);
  }
}

static std::filesystem::path GetSyncFosTempFilePath() {
  return FilePath(a()->sess().dirs().temp_directory(), "WWIVSYNC.ENV");
}

static std::filesystem::path GetDosXtrnPath() {
  return FilePath(a()->bindir(), "dosxtrn.exe");
}

static std::string CreateSyncFosCommandLine(const std::string& tempFilePath, int nSyncMode) {
  std::stringstream ss;
  ss << GetDosXtrnPath().string() << " " << tempFilePath << " " << "NT" << " ";
  ss << a()->sess().instance_number() << " " << nSyncMode << " " << CONST_SBBSFOS_LOOPS_BEFORE_YIELD;
  return ss.str();
}

// returns true if the file is deleted.
static bool DeleteSyncTempFile() {
  const auto fn = GetSyncFosTempFilePath();
  if (File::Exists(fn)) {
    File::Remove(fn);
    return true;
  }
  return false;
}

static bool CreateSyncTempFile(std::string *out, const std::string& commandLine) {
  out->assign(GetSyncFosTempFilePath().string());
  DeleteSyncTempFile();

  TextFile file(*out, "wt");
  if (!file.IsOpen()) {
    return false;
  }
  file.Write(commandLine);
  if (!file.Close()) {
    return false;
  }

  return true;
}

bool DoSyncFosLoopNT(HANDLE hProcess, HANDLE hSyncHangupEvent, HANDLE hSyncReadSlot, int nSyncMode) {
  LogToSync("Starting DoSyncFosLoopNT()\r\n");
  HANDLE hSyncWriteSlot = INVALID_HANDLE_VALUE;     // Mail Slot for writing

  // Cleanup on all exit paths
  auto at_exit = finally([&] {
    if (hSyncHangupEvent != INVALID_HANDLE_VALUE) { 
      CloseHandle(hSyncHangupEvent);
      hSyncHangupEvent = INVALID_HANDLE_VALUE;
    }
    if (hSyncReadSlot != INVALID_HANDLE_VALUE) { 
      CloseHandle(hSyncReadSlot); 
      hSyncReadSlot = INVALID_HANDLE_VALUE;
    }
    if (hSyncWriteSlot != INVALID_HANDLE_VALUE) { 
      CloseHandle(hSyncWriteSlot); 
      hSyncWriteSlot = INVALID_HANDLE_VALUE;
    }
  });

  char szReadBuffer[ CONST_SBBSFOS_BUFFER_SIZE + CONST_SBBSFOS_BUFFER_PADDING ];
  ::ZeroMemory(szReadBuffer, CONST_SBBSFOS_BUFFER_SIZE + CONST_SBBSFOS_BUFFER_PADDING);
  szReadBuffer[ CONST_SBBSFOS_BUFFER_SIZE + 1 ] = '\xFE';

  auto counter = 0;
  for (;;) {
    counter++;
    if (a()->sess().using_modem() && (!bout.remoteIO()->connected())) {
      SetEvent(hSyncHangupEvent);
      LogToSync("Setting Hangup Event and Sleeping\r\n");
      wwiv::os::sleep_for(1s);
    }

    if (counter > CONST_NUM_LOOPS_BEFORE_EXIT_CHECK) {
      // Only check for exit when we've been idle
      DWORD dwExitCode = 0;
      if (GetExitCodeProcess(hProcess, &dwExitCode)) {
        if (dwExitCode != STILL_ACTIVE) {
          // process is done and so are we.
          LogToSync("Process finished, exiting\r\n");
          return true;
        }
      }
    }

    if (bout.remoteIO()->incoming()) {
      counter = 0;
      // SYNCFOS_DEBUG_PUTS( "Char available to send to the door" );
      const auto nNumReadFromComm = bout.remoteIO()->read(szReadBuffer, CONST_SBBSFOS_BUFFER_SIZE);

      if (a()->IsExecLogSyncFoss()) {
        // LogToSync(StrCat("Read [", nNumReadFromComm, "] from comm\r\n"));
        for (unsigned int n_lp = 0; n_lp < nNumReadFromComm; n_lp++) {
          fprintf(hLogFile, "[%u]", static_cast<unsigned char>(szReadBuffer[n_lp]));
        }
        fprintf(hLogFile, "   ");
        for (unsigned int n_lp = 0; n_lp < nNumReadFromComm; n_lp++) {
          fprintf(hLogFile, "[%c]", static_cast<unsigned char>(szReadBuffer[ n_lp ]));
        }
        fprintf(hLogFile, "\r\n");
      }

      if (hSyncWriteSlot == INVALID_HANDLE_VALUE) {
        // Create Write handle.
        wwiv::os::sleep_for(500ms);
        auto write_slot_name = fmt::format(R"(\\.\mailslot\sbbsexec\wr{})", a()->sess().instance_number());
        LogToSync(StrCat("Creating Mail Slot: '", write_slot_name, "'\r\n"));

        hSyncWriteSlot = CreateFile(write_slot_name.c_str(),
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    static_cast<HANDLE>(nullptr));
        if (hSyncWriteSlot == INVALID_HANDLE_VALUE) {
          sysoplog(StrCat("!!! Unable to create mail slot for writing for SyncFoss External program: ", GetLastError()));
          LogToSync(StrCat("!!! Unable to create mail slot for writing for SyncFoss External program: ", GetLastError()));
          return false;
        }
        LogToSync("Created MailSlot\r\n");
      }

      if (nNumReadFromComm == 3 && static_cast<uint8_t>(szReadBuffer[0]) == 0xff) {
        // IAC, skip over them so we ignore them for now.
        // We do this in exec_unix.cpp too.
        LOG(INFO) << "IAC";
      } else {
        DWORD dwNumWrittenToSlot = 0;
        WriteFile(hSyncWriteSlot, szReadBuffer, nNumReadFromComm, &dwNumWrittenToSlot, nullptr);
        if (a()->IsExecLogSyncFoss()) {
          LogToSync(StrCat("Wrote [", dwNumWrittenToSlot, "] to MailSlot\r\n"));
        }
      }
    }

    int nBufferPtr      = 0;    // BufPtr
    DWORD dwNextSize    = 0;    // InUsed
    DWORD dwNumMessages = 0;    // TmpLong
    DWORD dwReadTimeOut = 0;    // -------

    if (GetMailslotInfo(hSyncReadSlot, nullptr, &dwNextSize, &dwNumMessages, &dwReadTimeOut)) {
      if (dwNumMessages > 0) {
        // Too verbose.
        if (a()->IsExecLogSyncFoss()) {
          LogToSync(StrCat("[", dwNumMessages, "/", std::min<DWORD>(dwNumMessages,
          CONST_SBBSFOS_BUFFER_SIZE - 1000), "] slot messages\r\n"));
        }
        dwNumMessages = std::min<DWORD>(dwNumMessages, CONST_SBBSFOS_BUFFER_SIZE - 1000);
        dwNextSize = 0;

        for (DWORD nSlotCounter = 0; nSlotCounter < dwNumMessages; nSlotCounter++) {
          if (ReadFile(hSyncReadSlot,
                       &szReadBuffer[ nBufferPtr ],
                       CONST_SBBSFOS_BUFFER_SIZE - nBufferPtr,
                       &dwNextSize,
                       nullptr)) {
            if (dwNextSize > 1) {
              LogToSync(StrCat("[", dwNextSize, "] bytes read   \r\n"));
            }
            if (dwNextSize == MAILSLOT_NO_MESSAGE) {
              LogToSync("** MAILSLOT thinks it's empty!\r\n");
            } else {
              nBufferPtr += dwNextSize;   // was just ++.. oops
            }

            if (nBufferPtr >= CONST_SBBSFOS_BUFFER_SIZE - 10) {
              break;
            }
          }
        }
      } else {
        wwiv::os::yield();
      }

      if (nBufferPtr > 0) {
        counter = 0;
        if (nSyncMode & CONST_SBBSFOS_DOSOUT_MODE) {
          // For some reason this doesn't write twice locally, so it works pretty well.
          szReadBuffer[nBufferPtr] = '\0';
          bout.outstr(szReadBuffer);

          // ExpandWWIVHeartCodes( szReadBuffer );
          // int nNumWritten = bout.remoteIO()->write( szReadBuffer, strlen( szReadBuffer )  );
        } else {
          const auto num_written = bout.remoteIO()->write(szReadBuffer, nBufferPtr);
          if (a()->IsExecLogSyncFoss()) {
            // Too verbose.
            LogToSync(StrCat("Wrote [", num_written, "] bytes to comm.\r\n"));
          }
        }
      }
      if (a()->IsExecLogSyncFoss()) {
        if (szReadBuffer[CONST_SBBSFOS_BUFFER_SIZE + 1] != '\xFE') {
          LogToSync("!!!!!! MEMORY CORRUPTION ON szReadBuffer !!!!!!!!\r\n\r\n\n\n");
        }
      }
    }
    if (counter > 0) {
      wwiv::os::yield();
    }
  }
}

static bool SetupSyncFoss(DWORD& dwCreationFlags, HANDLE& hSyncHangupEvent, HANDLE& hSyncReadSlot) {
  hSyncHangupEvent = INVALID_HANDLE_VALUE;
  hSyncReadSlot = INVALID_HANDLE_VALUE;     // Mail Slot for reading

  // Create each syncfoss window in it's own WOW VDM.
  dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;

  // Create Hangup Event.
  const auto event_name = fmt::format("sbbsexec_hungup{}", a()->sess().instance_number());
  hSyncHangupEvent = CreateEvent(nullptr, TRUE, FALSE, event_name.c_str());
  if (hSyncHangupEvent == INVALID_HANDLE_VALUE) {
    LogToSync(StrCat("!!! Unable to create Hangup Event for SyncFoss External program: ", GetLastError()));
    sysoplog(StrCat("!!! Unable to create Hangup Event for SyncFoss External program: ", GetLastError()));
    return false;
  }

  // Create Read Mail Slot
  const auto readslot_name = fmt::format(R"(\\.\mailslot\sbbsexec\rd{})", a()->sess().instance_number());
  hSyncReadSlot = CreateMailslot(readslot_name.c_str(), CONST_SBBSFOS_BUFFER_SIZE, 0, nullptr);
  if (hSyncReadSlot == INVALID_HANDLE_VALUE) {
    LogToSync(StrCat("!!! Unable to create mail slot for reading for SyncFoss External program: ", GetLastError()));
    sysoplog(StrCat("!!! Unable to create mail slot for reading for SyncFoss External program: ", GetLastError()));
    CloseHandle(hSyncHangupEvent);
    hSyncHangupEvent = INVALID_HANDLE_VALUE;
    return false;
  }
  return true;
}


// last_error as a std::string.
static std::string ErrorAsString(DWORD last_error) {
  LPSTR messageBuffer{nullptr};
  const auto size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, 
    last_error, 
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
    reinterpret_cast<LPSTR>(&messageBuffer), 
    0, 
    nullptr);

  std::string message{messageBuffer, size};

  // Free the buffer.
  LocalFree(messageBuffer);
  return fmt::format("({}): {}", last_error, message);
}
//  Main code that launches external programs and handle sbbsexec support

static int exec_cmdline_sync(wwiv::bbs::CommandLine& cmdline, int flags, int nSyncMode = 0) {
  const auto saved_binary_mode = bout.remoteIO()->binary_mode();

  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(STARTUPINFO);
  ZeroMemory(&pi, sizeof(pi));

  std::string syncFosTempFile;
  const auto user_command_line = cmdline.cmdline();
  if (!CreateSyncTempFile(&syncFosTempFile, user_command_line)) {
    return -1;
  }
  auto working_cmdline = CreateSyncFosCommandLine(syncFosTempFile, nSyncMode);

  const auto logfile_name = FilePath(a()->logdir(), "wwivsync.log");
  hLogFile = fopen(logfile_name.string().c_str(), "at");
  LogToSync(std::string(78, '='));
  LogToSync("\r\n\r\n");

  VLOG(1) << "exec_cmdline: working_commandline: " << working_cmdline;

  std::string title = (flags & EFLAG_NETPROG)
                          ? "NETWORK"
                          : fmt::format("{} in door on node {}", a()->user()->GetName(),
                                        a()->sess().instance_number());
  si.lpTitle = &title[0];

  auto hSyncHangupEvent = INVALID_HANDLE_VALUE; // NOLINT(readability-qualified-auto)
  auto hSyncReadSlot = INVALID_HANDLE_VALUE;    // Mailslot for reading
  DWORD dwCreationFlags = 0;
  SetupSyncFoss(dwCreationFlags, hSyncHangupEvent, hSyncReadSlot);
  wwiv::os::sleep_for(250ms);

  const auto current_directory = File::current_directory().string();

  // Need a non-const string for the commandline
  char szTempWorkingCommandline[MAX_PATH + 1];
  to_char_array(szTempWorkingCommandline, working_cmdline);
  const auto bRetCP =
      CreateProcess(nullptr, szTempWorkingCommandline, nullptr, nullptr, TRUE, dwCreationFlags,
                    nullptr, // a()->xenviron not using nullptr causes things to not work.
                    current_directory.c_str(), &si, &pi);

  if (!bRetCP) {
    const auto error_code = ::GetLastError();
    const auto error_message = ErrorAsString(error_code);
    LOG(ERROR) << "CreateProcessFailed: error: " << error_message;
    sysoplog(fmt::format("!!! CreateProcess failed for command: [{}] with Error Message: {}", working_cmdline, error_message));

    // Restore old binary mode.
    bout.remoteIO()->set_binary_mode(saved_binary_mode);
    return -1;
  }

  // Kinda hacky but WaitForInputIdle doesn't work on console application.
  wwiv::os::sleep_for(std::chrono::milliseconds(a()->GetExecChildProcessWaitTime()));
  const auto sleep_zero_times = 5;
  for (auto iter = 0; iter < sleep_zero_times; iter++) {
    wwiv::os::yield();
  }

  CloseHandle(pi.hThread);

  bout.remoteIO()->set_binary_mode(true);
  const auto sync_loop_status =
      DoSyncFosLoopNT(pi.hProcess, hSyncHangupEvent, hSyncReadSlot, nSyncMode);
  LogToSync(StrCat("DoSyncFosLoopNT: Returned ", sync_loop_status, "\r\n", std::string(78, '='),
                    "\r\n\r\n\r\n"));

  if (sync_loop_status) {
    DWORD dwExitCode = 0;
    GetExitCodeProcess(pi.hProcess, &dwExitCode);
    if (dwExitCode == STILL_ACTIVE) {
      LogToSync("Terminating Sync Process");
      LOG(INFO) << "Terminating Sync process.";
      TerminateProcess(pi.hProcess, 0);
    } else {
      LogToSync(fmt::format("Sync Process Exit Code: {}", dwExitCode));
    }
  }
  fclose(hLogFile);

  DWORD dwExitCode = 0;
  GetExitCodeProcess(pi.hProcess, &dwExitCode);

  // Close process and thread handles.
  CloseHandle(pi.hProcess);

  // Restore old binary mode.
  bout.remoteIO()->set_binary_mode(saved_binary_mode);

  return static_cast<int>(dwExitCode);
}

HANDLE stdin_read = nullptr;
HANDLE stdin_write = nullptr;
HANDLE stdout_read = nullptr;
HANDLE stdout_write = nullptr;
std::atomic<bool> stop_pipe;

static bool create_stdio_pipes() {
  stop_pipe.store(false);
  SECURITY_ATTRIBUTES saAttr{};

  // Set the bInheritHandle flag so pipe handles are inherited.
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  // Create a pipe for the child process's STDOUT.
  if (!CreatePipe(&stdout_read, &stdout_write, &saAttr, 0)) {
    return false;
  }

  // Ensure the read handle to the pipe for STDOUT is not inherited.
  if (!SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0)) {
    return false;
  }

  // Create a pipe for the child process's STDIN.
  if (!CreatePipe(&stdin_read, &stdin_write, &saAttr, 0)) {
    return false;
  }

  // Ensure the write handle to the pipe for STDIN is not inherited.
  if (!SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0)) {
    return false;
  }

  return true;
}

void pump_pipes(HANDLE hProcess) {
  static constexpr int check_process_every = 10;
  char buf[1024];
  int count = 0;
  while (!stop_pipe.load()) {
    DWORD num_read, num_avail, msg_left;
    if (PeekNamedPipe(stdout_read, buf, sizeof(buf), &num_read, &num_avail, &msg_left) &&
        num_read > 0) {
      auto b = ReadFile(stdout_read, buf, sizeof(buf), &num_read, NULL);
      if (!b || num_read == 0) {
        VLOG(1) << "Exiting pump_pipes: b:" << b << "; num_read: " << num_read;
        return;
      }
      bout.remoteIO()->write(buf, num_read);
    }

    if (!bout.remoteIO()->connected()) {
      VLOG(1) << "Exiting pump_pipes: Caller Hung up.";
      return;
    }

    if (bout.remoteIO()->incoming()) {
      if (num_read = bout.remoteIO()->read(buf, sizeof(buf)); num_read > 0) {
        DWORD num_written;
        if (!WriteFile(stdin_write, buf, num_read, &num_written, nullptr)) {
          VLOG(1) << "Exiting pump_pipes; Write to pipe failed";
          return;
        }
      }
    }

    if (++count >= check_process_every) {
      count = 0;
      DWORD dwExitCode;
      if (GetExitCodeProcess(hProcess, &dwExitCode) && dwExitCode != STILL_ACTIVE) {
        VLOG(1) << "Process exited. Ending pump_pipes";
        return;
      }
    }
  }
  Sleep(100);
}

int exec_cmdline(wwiv::bbs::CommandLine& cmdline, int flags) {
  auto mask = EFLAG_COMIO | EFLAG_NETFOSS | EFLAG_LISTEN_SOCK | EFLAG_STDIO | EFLAG_SYNC_FOSSIL | EFLAG_UNIX_SOCK;
  auto test = flags & mask;
  if ((test & (test - 1)) != 0) {
    // Not allowed.
    LOG(ERROR) << "Tried to execute command with multiple exec types: " << cmdline.cmdline();
    return false;
  }

  bool need_reopen_remoteio = false;
  STARTUPINFO si{};
  PROCESS_INFORMATION pi{};

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(STARTUPINFO);
  ZeroMemory(&pi, sizeof(pi));

  std::unique_ptr<wwiv::bbs::ExecSocket> exec_socket;

  auto working_cmdline = cmdline.cmdline();
  if (a()->sess().using_modem()) {
    if (flags & EFLAG_SYNC_FOSSIL) {
      return exec_cmdline_sync(cmdline, flags, 0);
    }
    if (flags & EFLAG_COMIO) {
      return exec_cmdline_sync(cmdline, flags,
                               CONST_SBBSFOS_DOSIN_MODE | CONST_SBBSFOS_DOSOUT_MODE);
    }

    if (flags & EFLAG_STDIO) {
      if (!create_stdio_pipes()) {
        LOG(ERROR) << "Unable to create pipes for STDIO";
        return -1;
      }
      // Set the socket to be std{in,out}
      // This doesn't work at all
      // const auto sock = bout.remoteIO()->GetDoorHandle();

      // si.hStdError = stdout_write;
      si.hStdOutput = stdout_write;
      si.hStdInput = stdin_read;
      si.dwFlags |= STARTF_USESTDHANDLES;
      // N.B. Don't close remote IO.
    } else if (flags & EFLAG_NETFOSS) {
      if (const auto nf_path = create_netfoss_bat()) {
        working_cmdline = fmt::format("{} {}", nf_path.value(), cmdline.cmdline());
        VLOG(1) << "Closing remote IO for NetFOSS";
        bout.remoteIO()->close(true);
        need_reopen_remoteio = true;
      } else {
        ssm(1) << "NetFoss is not installed properly.";
        LOG(ERROR) << "Failed to create NF.BAT for command: " << working_cmdline;
        bout.nl(2);
        bout.pl("|#6Please tell the SysOp to install NetFoss properly.");
        bout.nl(2);
        bout.pausescr();
        return false;
      }
    } else if (flags & EFLAG_LISTEN_SOCK) {
      uint16_t port = 0;
      exec_socket = std::make_unique<wwiv::bbs::ExecSocket>(port);
      // Add %Z into the commandline.
      if (exec_socket->listening()) {
        cmdline.add('Z', exec_socket->z());
        working_cmdline = cmdline.cmdline();
      }

    } else if (flags & EFLAG_UNIX_SOCK) {
      exec_socket = std::make_unique<wwiv::bbs::ExecSocket>(a()->sess().dirs().scratch_directory());
      // Add %Z into the commandline.
      if (exec_socket->listening()) {
        cmdline.add('Z', exec_socket->z());
        working_cmdline = cmdline.cmdline();
      }
    } else {
      // Not STDIO or NETFOSS so close remote IO  
      VLOG(1) << "Closing remote IO";
      bout.remoteIO()->close(true);
      need_reopen_remoteio = true;
    }
  }
  VLOG(1) << "exec_cmdline: working_commandline: " << working_cmdline;

  std::string title = (flags & EFLAG_NETPROG) 
    ? "NETWORK" : fmt::format("{} in door on node {}", a()->user()->GetName(), a()->sess().instance_number());

  si.lpTitle = &title[0];

  const auto current_directory = File::current_directory().string();

  // Need a non-const string for the commandline
  char szTempWorkingCommandline[MAX_PATH+1];
  to_char_array(szTempWorkingCommandline, working_cmdline);
  DWORD dwCreationFlags = 0;
  if (!CreateProcess(nullptr, szTempWorkingCommandline, nullptr, nullptr, TRUE, dwCreationFlags,
                     nullptr, // a()->xenviron not using nullptr causes things to not work.
                     current_directory.c_str(), &si, &pi)) {
    const auto error_code = ::GetLastError();
    const auto error_message = ErrorAsString(error_code);
    LOG(ERROR) << "CreateProcessFailed: error: " << error_message;
    sysoplog(fmt::format("!!! CreateProcess failed for command: [{}] with Error Message: {}", working_cmdline, error_message));

    // If we return here, we may have to reopen the communications port.
    if (a()->sess().ok_modem_stuff() && a()->sess().using_modem() && need_reopen_remoteio) {
      VLOG(1) << "Reopening comm (on createprocess error)";
      bout.remoteIO()->open();
    }
    // Restore old binary mode.
    return -1;
  }

  CloseHandle(pi.hThread);
  bool terminated = false;

  if (a()->sess().using_modem() && (flags & EFLAG_STDIO)) {
    CloseHandle(stdout_write);
    CloseHandle(stdin_read);
    pump_pipes(pi.hProcess);
  } else if (exec_socket) {
    if (const auto sock = exec_socket->accept()) {
      DCHECK(bout.remoteIO());
      const auto res = exec_socket->pump_socket(pi.hProcess, sock.value(), *bout.remoteIO());
      if (res != pump_socket_result_t::process_exit) {
        // sockets closed but process still running.
        VLOG(1) << "Sockets closed, process still running; Forcefully terminating pid: "
                << pi.hProcess;
        if (TerminateProcess(pi.hProcess, 127)) {
          terminated = true;
        }
      }
    } else {
      VLOG(1) << "Accept never happend; Forcefully terminating pid: " << pi.hProcess;
      if (TerminateProcess(pi.hProcess, 127)) {
        terminated = true;
      }
    }
  }

  if (!terminated) {
    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);
  }

  DWORD dwExitCode = 0;
  GetExitCodeProcess(pi.hProcess, &dwExitCode);

  // Close process handle.
  CloseHandle(pi.hProcess);

  // reengage comm stuff
  if (need_reopen_remoteio && a()->sess().ok_modem_stuff() && a()->sess().using_modem()) {
    VLOG(1) << "Reopening comm";
    bout.remoteIO()->open();
  }

  return static_cast<int>(dwExitCode);
}
