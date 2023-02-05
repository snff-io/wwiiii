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
#ifndef INCLUDED_COMMON_OUTPUT_H
#define INCLUDED_COMMON_OUTPUT_H

#include "common/iobase.h"
#include "common/language.h"
#include "fmt/printf.h"
#include "local_io/curatr_provider.h"
#include "sdk/wwivcolors.h"
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace wwiv::sdk::ansi {
class Ansi;
class LocalIOScreen;
}

namespace wwiv::common {
struct Interpreted;

class MacroContext;


typedef std::basic_ostream<char>&(ENDL_TYPE_O)(std::basic_ostream<char>&);

class SavedLine {
public:
  SavedLine(std::vector<std::pair<char, uint8_t>> l, int c) : line(std::move(l)), color(c) {}
  std::vector<std::pair<char, uint8_t>> line;
  int color;
};

/**
 * Creates the Output class responsible for displaying information both
 * locally and remotely.
 *
 * To use this class you must set the following:
 * - LocalIO
 * - RemoteIO
 * - Context Provider
 * - User Provider
 * [Optional] Instance Message Processor.
 *
 * These may be modified after being set, so RAII does not work.
 */
class Output final : public local::io::curatr_provider, public IOBase {
public:
  typedef std::function<MacroContext&()> macro_context_provider_t;
  Output();
  Output(const Output&) = delete;
  Output(Output&&) = delete;
  Output& operator=(const Output&) = delete;
  Output& operator=(Output&&) = delete;
  ~Output() override;

  /** Sets the LocalIO instance to use locally for output */
  void SetLocalIO(local::io::LocalIO* local_io) override;

  /** Sets the provider for the session context */
  // ReSharper disable once CppMemberFunctionMayBeConst
  void set_macro_context_provider(macro_context_provider_t c) { macro_context_provider_ = std::move(c); }

  /**
   * This sets the current color (both locally and remotely) to that
   * specified (in WWIV format).
   */
  void ansic(int wwiv_color);

  /** Resets all ANSI color attributes back to the default gray on black. */
  void ResetColors();

  /** Moves the cursor position to x, y.  Note this is 1 based, not 0. */
  void goxy(int x, int y);
  void Up(int num);
  void Down(int num);
  void Left(int num);
  void Right(int num);
  void SavePosition();
  void RestorePosition();
  void nl(int num_lines = 1);
  void bs();

  /**
   * This sets the current color (both locally and remotely) to that
   * specified (in IBM format).
   */
  void setc(int c);
  /**
   * This sets the current color (both locally and remotely) to that
   * specified (in IBM format).
   */
  void setc(wwiv::sdk::Color color);
  [[nodiscard]] std::string MakeColor(int wwiv_color);
  [[nodiscard]] std::string MakeSystemColor(int c) const;
  [[nodiscard]] std::string MakeSystemColor(sdk::Color color) const;

  /** Displays msg in a light bar header. */
  void litebar(const std::string& msg);

  /** Backspaces from the current cursor position to the beginning of a line */
  void backline();

  /**
   * Clears from the cursor to the end of the line using ANSI sequences.  If the user
   * does not have ansi, this this function does nothing.
   */
  void clreol(int ct = 0);

  /**
   * Moves the cursor to the beginning of the line and clears the whole like.
   * If the user does not have ansi, this this function does nothing.
   */
  void clear_whole_line();

  /**
   * Clears the local and remote screen using ANSI (if enabled), otherwise DEC 12
   */
  void cls();

  /**
   * This will make a reverse-video prompt line i characters long, repositioning
   * the cursor at the beginning of the input prompt area.  Of course, if the
   * user does not want ansi, this routine does nothing.
   */
  void mpl(int length);

  /**
   * Writes text at position (x, y) using the current color.
   *
   * Note that x and y are zero based and (0, 0) is the top left corner of the screen.
   *
   * Returns the number of characters displayed.
   */
  int PutsXY(int x, int y, const std::string& text);

  /**
   * Writes text at position (x, y) using the system color attribute specified by a.
   *
   * Note that x and y are zero based and (0, 0) is the top left corner of the screen.
   *
   * Returns the number of characters displayed.
   */
  int PutsXYSC(int x, int y, int a, const std::string& text);

  /**
   * Writes text at position (x, y) using the user color specified by color.
   *
   * Note that x and y are zero based and (0, 0) is the top left corner of the screen.
   *
   * Returns the number of characters displayed.
   */
  int PutsXYA(int x, int y, int color, const std::string& text);

  /**
   * Executes the movement commands (up, down, left, right) specified by r.
   */
  void do_movement(const Interpreted& r);

  /**
   * This function outoutstr a string of characters to the screen (and remotely
   * if applicable).  The com port is also checked first to see if a remote
   * user has hung up.  Returns the number of characters displayed.
   */
  int outstr(const std::string& text);

  /**
   * This function outoutstr a string of characters to the screen (and remotely if applicable) 
   * and follows with a newline.  The com port is also checked first to see if a remote user 
   * has hung up.  Returns the number of characters displayed.
   */
  int pl(const std::string& text);

  // Prints an abort-able string (contained in *text). Returns 1 in *abort if the
  // string was aborted, else *abort should be zero.
  int bpla(const std::string& text, bool* abort);

  /**
   * Displays s which checking for abort and next
   * @see checka
   * Note: bout.outstr means "Output String And Next".
   *
   * @param text The text to display
   * @param abort The abort flag (Output Parameter)
   * @param next The next flag (Output Parameter)
   */
  int outstr(const std::string& text, bool* abort, bool* next);

  template <class... Args> int printf(const char* format_str, Args&&... args) {
    // Process arguments
    return outstr(fmt::sprintf(format_str, std::forward<Args>(args)...));
  }

  template <typename... Args> int print(const char* format_str, Args&&... args) {
    // Process arguments
    try {
      return outstr(fmt::format(format_str, args...));
    } catch (const fmt::format_error& e) {
      LOG(ERROR) << "caught fmt::format_error" << e.what();
      LOG(ERROR) << "fmt::format_error original string: '" << format_str << "'";
      DLOG(FATAL) << "bout.print crash";
    }
    return outstr(format_str);
  }

  /**
   * Displays the text in the strings file referred to by key.
   */
  int str(const std::string& key);

  /**
   * Displays text slowly coming out of a spinning wheel using
   * {@code}
   * | / - \ | - \ - /
   * {@endcode}
   */
  void spin_outstr(const std::string& text, int color);

  /**
   * This function prints out a string, with a user-specifiable delay between
   * each character, and a user-definable pause after the entire string has
   * been printed, then it backspaces the string. The color is also definable.
   * The parameters are as follows:
   * 
   * Note: ANSI is not required.
   * 
   * Example:
   * 
   * back_outstr("This is an example.", 3, 20ms, 500ms);
   *
   * @param[in] text:       The string to print
   * @param[in] color:      The color of the string
   * @param[in] char_dly:   Delay between each character, in milliseconds
   * @param[in] string_dly: Delay between completion of string and backspacing
   */
  void back_outstr(const std::string& text, int color, std::chrono::duration<double> char_dly, std::chrono::duration<double> string_dly);

  /**
   * Displays the text using a rainbow effect, cycling through the 1st 8 colors.
   */
  void rainbow(const std::string& text);

  /**
   * Displays the text in the "strings file" referred to by key adding all
   * key/value pairs from 'map' into the unnamed context.
   * 
   * @param[in] key:     The key into the string file to locate the text to display
   * @param[in] map:     The map of replacable parameters to display using the pipe code 
   *                     variables.  for example to display a variable "foo", use: "|{foo}"
   */
  int str(const std::string& key, const std::map<std::string, std::string>& map) {
    // Process arguments
    const auto format_str = lang().value(key);
    if (!map.empty()) {
      context().add_context_variable("", map);
    }
    const int r = outstr(format_str);
    context().remove_context_variable("");
    return r;
  }

  int outchr(char c, bool use_buffer = false);

  /**
   * Writes any remaining buffered text remotely.
   */
  void flush();

  /**
   * writes a character remotely only, optionally buffering it possible.
   */
  void rputch(char ch, bool use_buffer = false);

  /**
   * Writes a string remotely only.
   */
  void rputs(const std::string& text);

  /**
   * Redraws the line to the screen.
   */
  bool RestoreCurrentLine(const SavedLine& line);

  /**
   * Saves the current line and returns it.
   */
  SavedLine SaveCurrentLine() const;

  /**
   * Purges the inbound buffer of any unprocessed characters.
   */
  void dump();

  /** Clears the number of lines displayed since last pause */
  void clear_lines_listed() { lines_listed_ = 0; }

  /** Gets the number of lines displayed since last pause */
  [[nodiscard]] int lines_listed() const noexcept { return lines_listed_; }

  /** Sets the number of lines displayed since last pause */
  void lines_listed(int l) { lines_listed_ = l; }

  /** Returns the current x position according to a best guess */
  int wherex() const;

  /** Redraws the current line */
  void RedrawCurrentLine();

  // ANSI helpers.
  void move_up_if_newline(int num_lines);

  // ANSI movement happened.
  [[nodiscard]] bool ansi_movement_occurred() const noexcept { return ansi_movement_occurred_; }
  void clear_ansi_movement_occurred() { ansi_movement_occurred_ = false; }

  // curatr_provider
  [[nodiscard]] uint8_t curatr() const noexcept override { return curatr_; }
  void curatr(int n) override { curatr_ = static_cast<uint8_t>(n); }

  // reset the state of Output
  void reset();

  //
  // Handles enabling, disabling, and status of MCI/pipe expressions.
  //
  [[nodiscard]] bool mci_enabled() const noexcept { return mci_enabled_; }
  void enable_mci() { mci_enabled_ = true; }
  void disable_mci() { mci_enabled_ = false; }
  void set_mci_enabled(bool e) { mci_enabled_ = e; }

  /** Pause output, displaying the [PAUSE] message, and wait a key to be hit. */
  void pausescr();

  //
  // PrintFile and friends.  Used to display a file (msg/ans/b&w/etc).
  //

 /**
   * Displays a file locally, meaning it pauses at the end.  In the future this
   * could use LIST.COM or spawn some other viewer program (it used to on dos)
   * 
   * @param[in] data: The filename, optionally without extension and `data` options to display.
   *                  See menu_data_and_options_t for more information on the options.
   */
  void print_local_file(const std::string& data);

 /**
   * Displays a file from either the menu set, language dir, or root gfiles directory, allowing
   * for options such pause or speed.
   * 
   * Example: `FOO BPS=2400`
   *
   * @param[in] data: The filename, optionally without extension and `data` options to display. 
   *                  See menu_data_and_options_t for more information on the options. 
   * @param[in] abortable: If `true` a caller can press a key to stop display.
   * @param[in] force_pause: pause on screen even when ansi movement would have caused
   *                         no pause to occur.
   */
  bool printfile(const std::string& data, bool abortable = true, bool force_pause = true);

 /**
   * Displays the file using the full path to locate it.
   *
   * Example: `/opt/wwiv/gfiles/foo.msg`
   *
   * @param[in] path: The full file path to display including both directory and file extension.
   * @param[in] abortable: If `true` a caller can press a key to stop display.
   * @param[in] force_pause: pause on screen even when ansi movement would have caused
   *                         no pause to occur.
   */
  bool printfile_path(const std::filesystem::path& file_path, bool abortable = true,
                      bool force_pause = true);

  /**
   * Displays a help file or an error that no help is available. A help file is a normal 
   * file displayed with printfile.
   * 
   * @param[in] filename: The file name, optionally without extension, to display.
   */
  bool print_help_file(const std::string& filename);


  /**
   * Displays a random file from either the menu set, language dir, or root gfiles 
   * directory, allowing for options such pause or speed.
   * 
   * A random file has an extension of ".0" through ".999" instead of `.ans`
   *
   * Example: `FOO BPS=2400`
   *
   * @param[in] data: The base filename (without extension) and `data` options to display.
   *                  See menu_data_and_options_t for more information on the options.
   */
  bool printfile_random(const std::string& data);

  /** Gets the current language instance. */
  [[nodiscard]] Language& lang();

  bool newline{true};

private:
  int lines_listed_{0};
  char GetKeyForPause();

  // This will pause output without ANSI, displaying the [PAUSE] message, and wait a key to be hit.
  // in pause.cpp
  void pausescr_noansi();

  std::string outchr_buffer_;
  std::vector<std::pair<char, uint8_t>> current_line_;
  int x_{0};
  // Means we need to reset the color before displaying our
  // next newline character.
  bool needs_color_reset_at_newline_{false};

  bool ansi_movement_occurred_{false};
  uint8_t curatr_{7};
  bool mci_enabled_{true};
  std::unique_ptr<sdk::ansi::LocalIOScreen> screen_;
  std::unique_ptr<sdk::ansi::Ansi> ansi_;
  mutable macro_context_provider_t macro_context_provider_;
  std::string current_menu_set_;
  std::unique_ptr<Language> lang_;
};

} // namespace wwiv::common 

// Extern for everyone else.
extern wwiv::common::Output bout;

#endif