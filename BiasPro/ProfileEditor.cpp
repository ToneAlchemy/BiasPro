#include "ProfileEditor.h"
#include "Config.h"
#include "StorageCore.h"  // profileLooksValid
#include <avr/pgmspace.h>

namespace {
  constexpr uint8_t ActionCount = 4;
  constexpr uint8_t CharTableLength = 38;
  constexpr uint16_t PanelColor = 0x2104;

  const char AllowedLabelChars[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789- ";

  void clearProfile(TubeProfile& profile) {
    for (uint8_t index = 0; index < DeviceConfig::TubeNameLength; ++index) {
      profile.label[index] = '\0';
    }

    profile.maxDissipationWatts = 25;
    profile.screenCurrentPermille = 60;
  }

  void copyProfile(TubeProfile& target, const TubeProfile& source) {
    for (uint8_t index = 0; index < DeviceConfig::TubeNameLength; ++index) {
      target.label[index] = source.label[index];
    }

    target.label[DeviceConfig::TubeNameLength - 1] = '\0';
    target.maxDissipationWatts = source.maxDissipationWatts;
    target.screenCurrentPermille = source.screenCurrentPermille;
  }

  void drawMenuRow(
    Adafruit_ST7735& tft,
    uint8_t row,
    const __FlashStringHelper* label,
    bool active
  ) {
    const int16_t y = 30 + (row * 19);
    tft.setTextColor(active ? ST7735_BLACK : ST7735_WHITE, active ? ST7735_YELLOW : ST7735_BLACK);
    if (active) {
      tft.fillRect(12, y - 3, 136, 14, ST7735_YELLOW);
    } else {
      tft.fillRect(12, y - 3, 136, 14, ST7735_BLACK);
    }

    tft.setCursor(18, y);
    tft.print(label);
  }

  void printFieldMarker(Adafruit_ST7735& tft, bool selected) {
    tft.print(selected ? F(">") : F(" "));
  }
}

void ProfileEditor::open(uint8_t selectedIndex, uint8_t profileCount) {
  mode_ = ProfileEditorMode::ActionMenu;
  actionIndex_ = 0;
  selectedIndex_ = selectedIndex;
  labelCursor_ = 0;
  editField_ = EditField::Label;
  repaintFrame_ = true;
  editorValuesPrimed_ = false;

  if (profileCount == 0) {
    selectedIndex_ = 0;
  } else if (selectedIndex_ >= profileCount) {
    selectedIndex_ = profileCount - 1;
  }

  clearProfile(working_);
}

void ProfileEditor::close() {
  mode_ = ProfileEditorMode::Inactive;
}

uint8_t ProfileEditor::selectedIndex() const {
  return selectedIndex_;
}

ProfileEditorResult ProfileEditor::handleInput(
  ButtonEvent event,
  TubeProfile* profiles,
  uint8_t& profileCount,
  uint8_t capacity
) {
  if (mode_ == ProfileEditorMode::Inactive || event == ButtonEvent::None) {
    return ProfileEditorResult::None;
  }

  if (mode_ == ProfileEditorMode::ActionMenu) {
    if (event == ButtonEvent::Left) {
      actionIndex_ = actionIndex_ == 0 ? ActionCount - 1 : actionIndex_ - 1;
    } else if (event == ButtonEvent::Right) {
      actionIndex_ = actionIndex_ + 1;
      if (actionIndex_ >= ActionCount) actionIndex_ = 0;
    } else if (event == ButtonEvent::Center) {
      if (actionIndex_ == 0) {
        if (profileCount > 0) {
          startEdit(profiles[selectedIndex_]);
        }
      } else if (actionIndex_ == 1) {
        // Refuse to enter Create mode when the store is full; the menu footer
        // already shows "Profiles 10/10". Previously the editor opened anyway
        // and reported Saved while saveWorkingProfile silently dropped the
        // new profile.
        if (profileCount < capacity) {
          startCreate();
        }
      } else if (actionIndex_ == 2) {
        if (profileCount > 0) {
          startDelete();
        }
      } else {
        close();
        return ProfileEditorResult::Cancelled;
      }
    } else if (event == ButtonEvent::LongCenter) {
      close();
      return ProfileEditorResult::Cancelled;
    }

    return ProfileEditorResult::None;
  }

  if (mode_ == ProfileEditorMode::CreateProfile || mode_ == ProfileEditorMode::EditProfile) {
    if (event == ButtonEvent::LongCenter) {
      close();
      return ProfileEditorResult::Cancelled;
    }

    if (event == ButtonEvent::Center) {
      if (editField_ == EditField::Label) {
        if (labelCursor_ < DeviceConfig::TubeNameLength - 2) {
          labelCursor_ += 1;
        } else {
          editField_ = EditField::Watts;
        }
      } else if (editField_ == EditField::Watts) {
        editField_ = EditField::ScreenPermille;
      } else if (editField_ == EditField::ScreenPermille) {
        editField_ = EditField::Save;
      } else {
        if (!profileLooksValid(working_)) {
          // Invalid name (e.g. every character blanked): bounce back to the
          // Name field instead of committing a record that would fail load-time
          // validation and wipe all custom profiles on the next boot.
          editField_ = EditField::Label;
          labelCursor_ = 0;
          editorValuesPrimed_ = false;
          return ProfileEditorResult::None;
        }
        saveWorkingProfile(profiles, profileCount, capacity);
        close();
        return ProfileEditorResult::Saved;
      }
    } else {
      applyEditEvent(event);
    }

    return ProfileEditorResult::None;
  }

  if (mode_ == ProfileEditorMode::DeleteConfirm) {
    if (event == ButtonEvent::Left || event == ButtonEvent::LongCenter) {
      mode_ = ProfileEditorMode::ActionMenu;
      repaintFrame_ = true;
    } else if (event == ButtonEvent::Right || event == ButtonEvent::Center) {
      deleteSelectedProfile(profiles, profileCount);
      close();
      return ProfileEditorResult::Deleted;
    }

    return ProfileEditorResult::None;
  }

  return ProfileEditorResult::None;
}

void ProfileEditor::draw(
  Adafruit_ST7735& tft,
  const TubeProfile* profiles,
  uint8_t profileCount
) {
  if (mode_ == ProfileEditorMode::ActionMenu) {
    drawActionMenu(tft, profileCount, repaintFrame_);
  } else if (mode_ == ProfileEditorMode::CreateProfile || mode_ == ProfileEditorMode::EditProfile) {
    drawEditScreen(tft, repaintFrame_);
  } else if (mode_ == ProfileEditorMode::DeleteConfirm) {
    drawDeleteConfirm(tft, profiles, profileCount);
  }
  repaintFrame_ = false;
}

void ProfileEditor::startCreate() {
  clearProfile(working_);
  working_.label[0] = 'N';
  working_.label[1] = 'E';
  working_.label[2] = 'W';
  working_.label[3] = '\0';

  mode_ = ProfileEditorMode::CreateProfile;
  editField_ = EditField::Label;
  labelCursor_ = 0;
  repaintFrame_ = true;
  editorValuesPrimed_ = false;
}

void ProfileEditor::startEdit(const TubeProfile& source) {
  copyProfile(working_, source);
  mode_ = ProfileEditorMode::EditProfile;
  editField_ = EditField::Label;
  labelCursor_ = 0;
  repaintFrame_ = true;
  editorValuesPrimed_ = false;
}

void ProfileEditor::startDelete() {
  mode_ = ProfileEditorMode::DeleteConfirm;
  repaintFrame_ = true;
  editorValuesPrimed_ = false;
}

void ProfileEditor::applyEditEvent(ButtonEvent event) {
  const int8_t direction =
    event == ButtonEvent::Right ? 1 :
    event == ButtonEvent::Left ? -1 : 0;

  if (direction == 0) {
    return;
  }

  if (editField_ == EditField::Label) {
    changeLabelCharacter(direction);
  } else if (editField_ == EditField::Watts) {
    changeWatts(direction);
  } else if (editField_ == EditField::ScreenPermille) {
    changeScreenPermille(direction);
  }
}

void ProfileEditor::changeLabelCharacter(int8_t direction) {
  char current = working_.label[labelCursor_];

  if (current == '\0') {
    current = ' ';
  }

  int8_t tableIndex = static_cast<int8_t>(currentCharTableIndex(current));
  tableIndex += direction;

  if (tableIndex < 0) {
    tableIndex = CharTableLength - 1;
  } else if (tableIndex >= CharTableLength) {
    tableIndex = 0;
  }

  working_.label[labelCursor_] = charAtTableIndex(static_cast<uint8_t>(tableIndex));
  working_.label[DeviceConfig::TubeNameLength - 1] = '\0';
}

void ProfileEditor::changeWatts(int8_t direction) {
  int16_t value = static_cast<int16_t>(working_.maxDissipationWatts) + direction;

  if (value < 1) value = 1;
  if (value > 99) value = 99;

  working_.maxDissipationWatts = static_cast<uint8_t>(value);
}

void ProfileEditor::changeScreenPermille(int8_t direction) {
  int16_t value = static_cast<int16_t>(working_.screenCurrentPermille) + (direction * 5);

  if (value < 0) value = 0;
  if (value > 300) value = 300;

  working_.screenCurrentPermille = static_cast<uint16_t>(value);
}

void ProfileEditor::saveWorkingProfile(
  TubeProfile* profiles,
  uint8_t& profileCount,
  uint8_t capacity
) {
  if (profiles == nullptr || capacity == 0) {
    return;
  }

  // Insurance: never let an invalid working profile reach the stored array,
  // even if a future caller bypasses the editor's bounce-to-Name gate.
  if (!profileLooksValid(working_)) {
    return;
  }

  if (mode_ == ProfileEditorMode::CreateProfile) {
    if (profileCount >= capacity) {
      return;
    }

    copyProfile(profiles[profileCount], working_);
    selectedIndex_ = profileCount;
    profileCount += 1;
    return;
  }

  if (selectedIndex_ < profileCount) {
    copyProfile(profiles[selectedIndex_], working_);
  }
}

void ProfileEditor::deleteSelectedProfile(TubeProfile* profiles, uint8_t& profileCount) {
  if (profiles == nullptr || profileCount == 0 || selectedIndex_ >= profileCount) {
    return;
  }

  for (uint8_t index = selectedIndex_; index + 1 < profileCount; ++index) {
    copyProfile(profiles[index], profiles[index + 1]);
  }

  profileCount -= 1;

  if (profileCount == 0) {
    selectedIndex_ = 0;
  } else if (selectedIndex_ >= profileCount) {
    selectedIndex_ = profileCount - 1;
  }
}

void ProfileEditor::drawActionMenu(Adafruit_ST7735& tft, uint8_t profileCount, bool fullFrame) {
  if (fullFrame) {
    tft.fillScreen(ST7735_BLACK);

    tft.setTextSize(1);
    tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
    tft.setCursor(5, 4);
    tft.print(F("PROFILE EDIT"));

    tft.fillRect(7, 20, 146, 1, PanelColor);
  }

  drawMenuRow(tft, 0, F("Modify"), actionIndex_ == 0);
  drawMenuRow(tft, 1, F("Create"), actionIndex_ == 1);
  drawMenuRow(tft, 2, F("Remove"), actionIndex_ == 2);
  drawMenuRow(tft, 3, F("Return"), actionIndex_ == 3);

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(5, 116);
  tft.print(F("Profiles "));
  tft.print(profileCount);
  tft.print(F("/"));
  tft.print(DeviceConfig::MaxTubeProfiles);
}

void ProfileEditor::drawEditScreen(Adafruit_ST7735& tft, bool fullFrame) {
  if (fullFrame) {
    tft.fillScreen(ST7735_BLACK);

    tft.setTextSize(1);
    tft.setTextColor(ST7735_CYAN, ST7735_BLACK);
    tft.setCursor(5, 4);

    if (mode_ == ProfileEditorMode::CreateProfile) {
      tft.print(F("NEW PROFILE"));
    } else {
      tft.print(F("EDIT PROFILE"));
    }

    tft.fillRect(4, 18, 152, 1, PanelColor);
    tft.setCursor(2, 118);
    tft.print(F("< > edit  C next"));
    editorValuesPrimed_ = false;
  }

  if (!editorValuesPrimed_) {
    drawEditRow(tft, EditField::Label, editField_ == EditField::Label);
    drawEditRow(tft, EditField::Watts, editField_ == EditField::Watts);
    drawEditRow(tft, EditField::ScreenPermille, editField_ == EditField::ScreenPermille);
    drawEditRow(tft, EditField::Save, editField_ == EditField::Save);
  } else if (editField_ != cachedEditField_) {
    drawEditRow(tft, cachedEditField_, false);
    drawEditRow(tft, editField_, true);
  } else {
    drawEditRow(tft, editField_, true);
  }

  cachedEditField_ = editField_;
  editorValuesPrimed_ = true;
}

void ProfileEditor::drawEditRow(Adafruit_ST7735& tft, EditField field, bool selected) {
  uint16_t color = selected ? ST7735_YELLOW : ST7735_WHITE;
  int16_t y = 30;

  if (field == EditField::Watts) {
    y = 52;
  } else if (field == EditField::ScreenPermille) {
    y = 72;
  } else if (field == EditField::Save) {
    color = selected ? ST7735_GREEN : ST7735_WHITE;
    y = 94;
  }

  tft.setTextSize(1);
  tft.setTextColor(color, ST7735_BLACK);
  tft.setCursor(10, y);
  printFieldMarker(tft, selected);

  if (field == EditField::Label) {
    tft.print(F(" Name "));
    tft.print(working_.label);
    tft.print(F("  "));
    tft.fillRect(52, 41, 36, 1, ST7735_BLACK);
    if (selected) {
      tft.fillRect(52 + (labelCursor_ * 6), 41, 5, 1, ST7735_YELLOW);
    }
  } else if (field == EditField::Watts) {
    tft.print(F(" Max "));
    tft.print(working_.maxDissipationWatts);
    tft.print(F("W "));
  } else if (field == EditField::ScreenPermille) {
    tft.print(F(" Scr "));
    tft.print(working_.screenCurrentPermille);
    tft.print(F("/1000  "));
  } else {
    tft.print(F(" Store "));
  }
}

void ProfileEditor::drawDeleteConfirm(
  Adafruit_ST7735& tft,
  const TubeProfile* profiles,
  uint8_t profileCount
) {
  tft.fillScreen(ST7735_RED);

  tft.setTextSize(2);
  tft.setTextColor(ST7735_WHITE, ST7735_RED);
  tft.setCursor(18, 16);
  tft.print(F("REMOVE?"));

  tft.setTextSize(1);
  tft.setCursor(20, 50);
  tft.print(F("Profile: "));

  if (profiles != nullptr && profileCount > 0 && selectedIndex_ < profileCount) {
    tft.print(profiles[selectedIndex_].label);
  } else {
    tft.print(F("none"));
  }

  tft.setCursor(14, 82);
  tft.print(F("Left cancel"));

  tft.setCursor(14, 100);
  tft.print(F("Right/C confirm"));
}

uint8_t ProfileEditor::currentCharTableIndex(char value) const {
  for (uint8_t index = 0; index < CharTableLength; ++index) {
    if (charAtTableIndex(index) == value) {
      return index;
    }
  }

  return CharTableLength - 1;
}

char ProfileEditor::charAtTableIndex(uint8_t index) const {
  if (index >= CharTableLength) {
    index = CharTableLength - 1;
  }

  return static_cast<char>(pgm_read_byte(&AllowedLabelChars[index]));
}
