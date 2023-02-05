/**************************************************************************/
/*                                                                        */
/*                            WWIV Version 5                              */
/*           Copyright (C)2020-2022, WWIV Software Services               */
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
#ifndef INCLUDED_SDK_FILES_FILES_H
#define INCLUDED_SDK_FILES_FILES_H

#include "dirs.h"
#include "core/clock.h"
#include "sdk/config.h"
#include "sdk/files/file_record.h"
#include "sdk/files/files_ext.h"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wwiv::sdk::files {

class FileArea;

enum class FileAreaSortType {
  FILENAME_ASC,
  FILENAME_DESC,
  DATE_ASC,
  DATE_DESC
};

class FileApi {
public:
  virtual ~FileApi() = default;
  explicit FileApi(const std::filesystem::path& data_directory);

  [[nodiscard]] bool Exist(const std::string& filename) const;
  [[nodiscard]] bool Exist(const directory_t& dir) const;
  [[nodiscard]] bool Create(const std::string& filename);
  [[nodiscard]] bool Create(const directory_t& dir);
  [[nodiscard]] bool Remove(const std::string& filename);
  [[nodiscard]] std::unique_ptr<FileArea> Open(const std::string& filename);
  [[nodiscard]] std::unique_ptr<FileArea> Open(const directory_t& dir);
  [[nodiscard]] std::unique_ptr<FileArea> CreateOrOpen(const directory_t& dir);

  [[nodiscard]] const core::Clock* clock() const noexcept;
  void set_clock(std::unique_ptr<core::Clock> clock);

private:
  const std::filesystem::path data_directory_;
  std::unique_ptr<core::Clock> clock_;
};

/**
 * Represents a file area header.
 * Internally this is stuffed into an uploadsrec re-purposing some
 * of the fields:
 *
 * filename must contain "|MARKER|"
 * numbytes is the total number of files in this area.
 * daten is the date of the newest file.
 */
class FileAreaHeader {
public:
  explicit FileAreaHeader(const uploadsrec& u);
  ~FileAreaHeader() = default;

  [[nodiscard]] uint32_t num_files() const { return u_.numbytes; }
  bool set_num_files(uint32_t n) { u_.numbytes = n; return true; }
  bool FixHeader(const core::Clock& clock, uint32_t num_files);
  uploadsrec& u() { return u_; }
  void set_daten(daten_t d);
  [[nodiscard]] daten_t daten() const;
private:
  uploadsrec u_;
};

class FileArea final {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = FileRecord;
  using difference_type = int;
  using pointer = FileRecord*;
  using reference = FileRecord&;

  FileArea(FileApi* api, const std::filesystem::path& data_directory, const directory_t& dir);
  FileArea(FileApi* api, const std::filesystem::path& data_directory, const std::string& filename);
  ~FileArea() = default;
  
  // File Header specific
  bool FixFileHeader();
  [[nodiscard]] FileAreaHeader& header() const;

  // File Dir Specific Operations

  // Loads the data on disk to the FileArea class
  bool Load();
  // Saves all changes to Disk. Note that extended
  // description changes happen immediately.
  bool Save();
  // Saves (if dirty and open) and marks this file areas as closed.
  bool Close();
  bool Lock();
  bool Unlock();
  [[nodiscard]] int number_of_files() const;

  // Sorts the files in memory.
  bool Sort(FileAreaSortType type);

  // File specific
  /** Optionally returns the file position for any exact match */
  std::optional<int> FindFile(const FileRecord& f);
  /** Optionally returns the file position for any exact match */
  std::optional<int> FindFile(const FileName& f);
  /** Optionally returns the file position for any exact match */
  std::optional<int> FindFile(const std::string& file_name);

  /** Searches for a file mask on an aligned file. i.e. 'FOO?????.ZIP' */
  std::optional<int> SearchFile(const std::string& filemask, int start_num = 1);

  FileRecord ReadFile(int num);
  bool AddFile(const FileRecord& f);
  bool AddFile(FileRecord& f, const std::string& ext_desc);
  bool UpdateFile(FileRecord& f, int num);
  bool UpdateFile(FileRecord& f, int num, const std::string& ext_desc);
  bool DeleteFile(const FileRecord& f, int file_number);
  bool DeleteFile(int file_number);

  // Extended Descriptions
  // Gets or creates the FileAreaExtendedDesc class.  If reload is true then
  // the class will be reloaded.
  std::optional<FileAreaExtendedDesc*> ext_desc(bool reload = false);
  // Adds an extended description to file f at pos num.  If num is -1
  // then don't update f.
  bool AddExtendedDescription(FileRecord& f, int num, const std::string& text);
  bool AddExtendedDescription(const std::string& file_name, const std::string& text);
  bool AddExtendedDescription(const FileRecord& f, const std::string& text);
  bool AddExtendedDescription(const FileName& f, const std::string& text);
  bool DeleteExtendedDescription(FileRecord& f, int num);
  bool DeleteExtendedDescription(const std::string& file_name);
  bool DeleteExtendedDescription(const FileName& f);
  std::optional<std::string> ReadExtendedDescriptionAsString(FileName& f);
  std::optional<std::string> ReadExtendedDescriptionAsString(FileRecord& f);
  std::optional<std::string> ReadExtendedDescriptionAsString(const std::string& aligned_name);

  // Gets the raw files
  [[nodiscard]] const std::vector<uploadsrec>& raw_files() const;
  // Sets the raw files.  Do not use unless you are doing a "fix" type tool
  [[nodiscard]] bool set_raw_files(std::vector<uploadsrec>);
  [[nodiscard]] std::filesystem::path path() const noexcept;
  [[nodiscard]] std::filesystem::path ext_path();

protected:
  bool ValidateFileNum(const FileRecord& f, int num);

  // Not owned.
  FileApi* api_;
  const std::filesystem::path data_directory_;
  const std::string base_filename_;
  const std::string filename_;
  directory_t dir_{};

  bool dirty_{false};
  bool open_{false};
  std::vector<uploadsrec> files_;

  std::unique_ptr<FileAreaHeader> header_;
  std::unique_ptr<FileAreaExtendedDesc> ext_desc_;
};

/**
 * Return true if two files {l, r} match each other either exactly or using
 * aligned wildcards.
 *
 * An aligned wildcard is of the form: "[A-Z? ]{8}\.[A-Z? ]{3}"
 */
bool aligned_wildcard_match(const std::string& l, const std::string& r);

} // namespace wwiv::sdk::files

#endif  // __INCLUDED_SDK_FILES_FILES_H__
