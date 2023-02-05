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
#include "bbs/message_file.h"

#include "bbs/bbs.h"
#include "common/output.h"
#include "core/file.h"
#include "core/strings.h"
#include "fmt/printf.h"
#include "local_io/keycodes.h"
#include "sdk/config.h"
#include "sdk/filenames.h"
#include "sdk/msgapi/type2_text.h"
#include "sdk/status.h"
#include <memory>
#include <string>

using namespace wwiv::core;
using namespace wwiv::sdk;
using namespace wwiv::sdk::msgapi;
using namespace wwiv::strings;

template <class S>
constexpr auto MSG_STARTING(S section) { return section * GATSECLEN + GAT_SECTION_SIZE; }

static long gat_section = -1;
static gati_t *gat = new gati_t[2048]();

/**
* Opens the message area file {messageAreaFileName} and returns the file handle.
* Note: This is a Private method to this module.
*/
static std::unique_ptr<File> OpenMessageFile(const std::string messageAreaFileName) {
  a()->status_manager()->reload_status();

  const auto filename =
      FilePath(a()->config()->msgsdir(), StrCat(messageAreaFileName, FILENAME_DAT_EXTENSION));
  auto file = std::make_unique<File>(filename);
  if (!file->Open(File::modeReadWrite | File::modeBinary)) {
    // Create message area file if it doesn't exist.
    file->Open(File::modeBinary | File::modeCreateFile | File::modeReadWrite);
    for (int i = 0; i < GAT_NUMBER_ELEMENTS; i++) {
      gat[i] = 0;
    }
    file->Write(gat, GAT_SECTION_SIZE);
    file->Close();
    file->set_length(GAT_SECTION_SIZE + (75L * 1024L));
    file->Open(File::modeReadWrite | File::modeBinary);
    gat_section = 0;
  }
  file->Seek(0L, File::Whence::begin);
  file->Read(gat, GAT_SECTION_SIZE);

  gat_section = 0;
  return file;
}

static void set_gat_section(File& file, int section) {
  if (gat_section == section) {
    return;
  }
  auto file_size = file.length();
  const auto section_pos = section * GATSECLEN;
  if (file_size < section_pos) {
    file.set_length(section_pos);
    file_size = section_pos;
  }
  file.Seek(section_pos, File::Whence::begin);
  if (file_size < (section_pos + GAT_SECTION_SIZE)) {
    for (auto i = 0; i < GAT_NUMBER_ELEMENTS; i++) {
      gat[i] = 0;
    }
    file.Write(gat, GAT_SECTION_SIZE);
  } else {
    file.Read(gat, GAT_SECTION_SIZE);
  }
  gat_section = section;
}

static void save_gat(File& file) {
  const auto section_pos = gat_section * GATSECLEN;
  file.Seek(section_pos, File::Whence::begin);
  file.Write(gat, GAT_SECTION_SIZE);
  a()->status_manager()->Run([](Status& s) {
    s.increment_filechanged(Status::file_change_posts);
  });
}

/**
* Deletes a message
* This is a public function.
*/
void remove_link(const messagerec* msg, const std::string& fileName) {
  switch (msg->storage_type) {
  case 0:
  case 1:
    break;
  case 2:
  {
    auto file(OpenMessageFile(fileName));
    if (!file->IsOpen()) {
      return;
    }
    set_gat_section(*file, static_cast<int>(msg->stored_as / GAT_NUMBER_ELEMENTS));
    uint32_t current_section = msg->stored_as % GAT_NUMBER_ELEMENTS;
    while (current_section > 0 && current_section < GAT_NUMBER_ELEMENTS) {
      const int next_section = static_cast<long>(gat[current_section]);
      gat[current_section] = 0;
      current_section = next_section;
    }
    save_gat(*file);
    file->Close();
  }
  break;
  default:
    // illegal storage type
    break;
  }
}

void savefile(const std::string& text, messagerec* msg, const std::string& fileName) {
  switch (msg->storage_type) {
  case 0:
  case 1:
    break;
  case 2:
  {
    gati_t gati[128];
    memset(&gati, 0, sizeof(gati));
    auto file(OpenMessageFile(fileName));
    if (file->IsOpen()) {
      for (int section = 0; section < 1024; section++) {
        set_gat_section(*file, section);
        int gatp = 0;
        int nNumBlocksRequired = static_cast<int>((text.length() + 511L) / MSG_BLOCK_SIZE);
        gati_t i4 = 1;
        while (gatp < nNumBlocksRequired && i4 < GAT_NUMBER_ELEMENTS) {
          if (gat[i4] == 0) {
            gati[gatp++] = i4;
          }
          ++i4;
        }
        if (gatp >= nNumBlocksRequired) {
          gati[gatp] = static_cast<gati_t>(-1);
          const auto text_len = ssize(text);
          for (int i = 0; i < nNumBlocksRequired; i++) {
            char block[MSG_BLOCK_SIZE + 1];
            memset(block, 0, sizeof(block));
            file->Seek(MSG_STARTING(gat_section) + MSG_BLOCK_SIZE * static_cast<long>(gati[i]), File::Whence::begin);
            const auto remaining = std::min(text_len - (i * MSG_BLOCK_SIZE), MSG_BLOCK_SIZE);
            memcpy(block, &text[i * MSG_BLOCK_SIZE], remaining);
            file->Write(block, MSG_BLOCK_SIZE);
            gat[gati[i]] = gati[i + 1];
          }
          save_gat(*file);
          break;
        }
      }
      file->Close();
    }
    msg->stored_as = static_cast<long>(gati[0]) + static_cast<long>(gat_section) * GAT_NUMBER_ELEMENTS;
  }
  break;
  default:
  {
    bout.printf("WWIV:ERROR:msgbase.cpp: Save - storage_type=%u!\r\n", msg->storage_type);
  }
  break;
  }
}

std::optional<std::string> readfile(const messagerec* msg, const std::string& fileName) {
  if (msg->storage_type != 2) {
    return std::nullopt;
  }

  auto file(OpenMessageFile(fileName));
  set_gat_section(*file, static_cast<int>(msg->stored_as) / GAT_NUMBER_ELEMENTS);
  auto current_section = static_cast<int>(msg->stored_as % GAT_NUMBER_ELEMENTS);
  long message_length = 0;
  while (current_section > 0 && current_section < GAT_NUMBER_ELEMENTS) {
    message_length += MSG_BLOCK_SIZE;
    current_section = gat[current_section];
  }
  if (message_length == 0) {
    bout.outstr("\r\nNo message found.\r\n\n");
    return std::nullopt;
  }

  std::string out;
  current_section = static_cast<int>(msg->stored_as) % GAT_NUMBER_ELEMENTS;
  while (current_section > 0 && current_section < GAT_NUMBER_ELEMENTS) {
    file->Seek(MSG_STARTING(gat_section) + MSG_BLOCK_SIZE * static_cast<uint32_t>(current_section), File::Whence::begin);
    char b[MSG_BLOCK_SIZE + 1];
    file->Read(b, MSG_BLOCK_SIZE);
    b[MSG_BLOCK_SIZE] = 0;
    out.append(b);
    current_section = gat[current_section];
  }
  file->Close();
  const auto last_cz = out.find_last_of(wwiv::local::io::CZ);
  const auto last_block_start = out.size() - MSG_BLOCK_SIZE;
  if (last_cz != std::string::npos && last_block_start >= 0 && last_cz > last_block_start) {
    // last block has a Control-Z in it.  Make sure we add a 0 after it.
    out.resize(last_cz);
  }
  return {out};
}

void lineadd(const messagerec* msg, const std::string& sx, std::string fileName) {
  const auto line = fmt::sprintf("%s\r\n\x1a", sx);

  switch (msg->storage_type) {
  case 0:
  case 1:
    break;
  case 2:
  {
    auto message_file(OpenMessageFile(fileName));
    set_gat_section(*message_file, msg->stored_as / GAT_NUMBER_ELEMENTS);
    int new1 = 1;
    while (new1 < GAT_NUMBER_ELEMENTS && gat[new1] != 0) {
      ++new1;
    }
    auto i = static_cast<int>(msg->stored_as % GAT_NUMBER_ELEMENTS);
    while (gat[i] != 65535) {
      i = gat[i];
    }
    char *b = nullptr;
    // The +1 may not be needed but BbsAllocA did it.
    if ((b = static_cast<char*>(calloc(GAT_NUMBER_ELEMENTS + 1, 1))) == nullptr) {
      message_file->Close();
      return;
    }
    message_file->Seek(MSG_STARTING(gat_section) + static_cast<long>(i) * MSG_BLOCK_SIZE, File::Whence::begin);
    message_file->Read(b, MSG_BLOCK_SIZE);
    int j = 0;
    while (j < MSG_BLOCK_SIZE && b[j] != wwiv::local::io::CZ) {
      ++j;
    }
    strcpy(&(b[j]), line.c_str());
    message_file->Seek(MSG_STARTING(gat_section) + static_cast<long>(i) * MSG_BLOCK_SIZE, File::Whence::begin);
    message_file->Write(b, MSG_BLOCK_SIZE);
    if (j + line.size() > MSG_BLOCK_SIZE && new1 != GAT_NUMBER_ELEMENTS) {
      message_file->Seek(MSG_STARTING(gat_section) + static_cast<long>(new1)  * MSG_BLOCK_SIZE, File::Whence::begin);
      message_file->Write(b + MSG_BLOCK_SIZE, MSG_BLOCK_SIZE);
      gat[new1] = 65535;
      gat[i] = static_cast<gati_t>(new1);
      save_gat(*message_file);
    }
    free(b);
    message_file->Close();
  }
  break;
  default:
    // illegal storage type
    break;
  }
}
