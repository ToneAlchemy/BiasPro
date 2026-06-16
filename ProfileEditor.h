#pragma once

#include <Arduino.h>
#include <Adafruit_ST7735.h>
#include "Config.h"
#include "DomainTypes.h"

enum class ProfileEditorMode : uint8_t {
  Inactive,
  ActionMenu,
  CreateProfile,
  EditProfile,
  DeleteConfirm
};

enum class ProfileEditorResult : uint8_t {
  None,
  Saved,
  Deleted,
  Cancelled
};

class ProfileEditor {
public:
  void open(uint8_t selectedIndex, uint8_t profileCount);
  void close();

  uint8_t selectedIndex() const;

  ProfileEditorResult handleInput(
    ButtonEvent event,
    TubeProfile* profiles,
    uint8_t& profileCount,
    uint8_t capacity
  );

  void draw(
    Adafruit_ST7735& tft,
    const TubeProfile* profiles,
    uint8_t profileCount
  );

private:
  enum class EditField : uint8_t {
    Label,
    Watts,
    ScreenPermille,
    Save
  };

  ProfileEditorMode mode_ = ProfileEditorMode::Inactive;
  EditField editField_ = EditField::Label;

  uint8_t actionIndex_ = 0;
  uint8_t selectedIndex_ = 0;
  uint8_t labelCursor_ = 0;
  bool repaintFrame_ = true;
  bool editorValuesPrimed_ = false;
  EditField cachedEditField_ = EditField::Label;

  TubeProfile working_;

  void startCreate();
  void startEdit(const TubeProfile& source);
  void startDelete();

  void applyEditEvent(ButtonEvent event);
  void changeLabelCharacter(int8_t direction);
  void changeWatts(int8_t direction);
  void changeScreenPermille(int8_t direction);

  void saveWorkingProfile(TubeProfile* profiles, uint8_t& profileCount, uint8_t capacity);
  void deleteSelectedProfile(TubeProfile* profiles, uint8_t& profileCount);

  void drawActionMenu(Adafruit_ST7735& tft, uint8_t profileCount, bool fullFrame);
  void drawEditScreen(Adafruit_ST7735& tft, bool fullFrame);
  void drawEditRow(Adafruit_ST7735& tft, EditField field, bool selected);
  void drawDeleteConfirm(Adafruit_ST7735& tft, const TubeProfile* profiles, uint8_t profileCount);

  uint8_t currentCharTableIndex(char value) const;
  char charAtTableIndex(uint8_t index) const;
};
