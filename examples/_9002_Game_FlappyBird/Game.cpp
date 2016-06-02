#include "Game.h"

Game::Game() {
  gameIndex = GAME_UNKNOWN;
  isRequestingExit = false;

  // game time
  lastFrameTime = millis();
  elapsedTime = 0.0f;
  blinkTime = 0.0f;

  // frame rate
  frameRate = 0;
  frameCount = 0l;
  fpsLastFrameTime = lastFrameTime;
  isFPSVisibled = false;
  memset(&fpsDigit, 0, sizeof(fpsDigit));

  // button
  isGamepadEnabled = false;
  buttonDelay = 0.0f;
  isButtonPressed[numberOfButtons] = {false};
  pressedButton = BUTTON_NONE;
  isButtonAllowed = true;
  memset(buttonPin, -1, sizeof(buttonPin));

#ifdef ARDUINO_ESP8266_ESPRESSO_LITE_V1
  buttonPin[0] = 0; // USER
  buttonPin[1] = 2; // FLASH
#else
  buttonPin[0] = 0; // GPIO0
  buttonPin[1] = 13; // GPIO13
#endif

  // sound
  soundDuration = 0.0f;
  nextSound = 0;
  nextSoundDelay = 0.0f;
  volume = 1.0f; // 0.0 to 1.0
  isVolumeChanged = false;
  isSoundInterruptEnabled = true;
  isSoundEnabled = true;
  readVolume();

  // battery
  battery = 0.0f; // 0.0 to 1.0
  batteryVoltage = 0.0f;

  // score
  highScore = 0l;
  score = 0l;
  highScoreAddress = -1;
}

void Game::drawBitmap(int x, int y, int width, int height, const uint8_t* bitmap, const uint8_t* mask, int color) {
  if (mask) {
    espert->oled.setColor(1 - color);
    espert->oled.drawBitmap(x, y, width, height, mask, false);
  }

  espert->oled.setColor(color);
  espert->oled.drawBitmap(x, y, width, height, bitmap, false);
}

String Game::floatToString(float value, int length, int decimalPalces) {
  String stringValue = String(value, decimalPalces);
  String prefix = "";

  for (int i = 0; i < length - stringValue.length(); i++) {
    prefix += " ";
  }

  return prefix + stringValue;
}

Game::GameIndex Game::getGameIndex() {
  return gameIndex;
}

int Game::getHighScoreAddress() {
  int address = -1;

  if (gameIndex != GAME_UNKNOWN) {
    address = saveDataHeaderSize;

    for (int i = numberOfGames - 1; i > gameIndex; i--) {
      address += String(maxScore[i]).length();

      if (i == GAME_OCTOPUS) {
        address += String(maxScore[i]).length(); // game B
      }
    }
  }

  return address;
}

void Game::init(ESPert* e, bool menu) {
  isMenuEnabled = menu;
  batteryVoltage = analogRead(A0);
  isGamepadEnabled = (batteryVoltage > 5) ? true : false;

  espert = e;
  espert->oled.init();
  espert->buzzer.init(isGamepadEnabled ? 15 : 12);
  espert->buzzer.on(1);
  espert->buzzer.on(0);

  int gamepadPin[numberOfButtons] = {12, 13, 14, 2, 0, A0}; // (left, right, up, down, a, b)
  for (int i = 0; i < numberOfButtons; i++) {
    if (isGamepadEnabled) {
      buttonPin[i] = gamepadPin[i];
    }

    if (buttonPin[i] != -1) {
      button[i].init(buttonPin[i], INPUT_PULLUP);
    }
  }

  randomSeed(analogRead(0));
  lastFrameTime = millis();
  fpsLastFrameTime = lastFrameTime;
}

bool Game::isBackToMenuEnabled() {
  return false;
}

bool Game::isBlink(float factor) {
  int speed = maxBlinkTime * factor;
  return ((int)blinkTime % speed < speed * 0.5f) ? true : false;
}

bool Game::isExit() {
  return isRequestingExit;
}

float Game::lerp(float t, float v0, float v1) {
  v0 = (1.0f - t) * v0 + t * v1;

  if (fabs(v1 - v0) < 1.0f) {
    v0 = v1;
  }

  return v0;
}

String Game::longToString(unsigned long value, int length, String prefixChar) {
  String stringValue = String(value);
  String prefix = "";

  for (int i = 0; i < length - stringValue.length(); i++) {
    prefix += prefixChar;
  }

  return prefix + stringValue;
}

void Game::playSound(int index) {
}

void Game::pressButton() {
}

unsigned long Game::readHighScore(int offset) {
  if (gameIndex != GAME_UNKNOWN) {
    if (espert->eeprom.read(0, saveDataKey.length()) == saveDataKey) {
      highScoreAddress = getHighScoreAddress();

      if (highScoreAddress != -1) {
        String data = espert->eeprom.read(highScoreAddress + offset, String(maxScore[gameIndex]).length());
        highScore = data.toInt();
        if (highScore < 0l || highScore > maxScore[gameIndex]) {
          highScore = constrain(highScore, 0, maxScore[gameIndex]);
          writeHighScore(offset);
        }
      }
    } else {
      for (int i = 0; i < 512; i++) {
        EEPROM.write(i, 0);
      }

      espert->eeprom.write(0, saveDataKey);
      highScore = 0l;
      writeHighScore(offset);
    }
  }

  return highScore;
}

void Game::readVolume() {
  if (espert->eeprom.read(0, saveDataKey.length()) == saveDataKey) {
    String data = espert->eeprom.read(saveDataKey.length(), volumeLength);
    volume = data.toFloat();
    if (volume < 0.0f || volume > 1.0f) {
      volume = constrain(volume, 0.0f, 1.0f);
      writeVolume();
    }
  } else {
    for (int i = 0; i < 512; i++) {
      EEPROM.write(i, 0);
    }

    espert->eeprom.write(0, saveDataKey);
    volume = 1.0f;
    writeVolume();
  }
}

void Game::render() {
}

void Game::renderBattery(int x, int y, int color) {
  if (isGamepadEnabled) {
    int batteryRead = analogRead(A0);

    if (batteryRead > 5) {
      batteryVoltage = lerp(0.01f, batteryVoltage, batteryRead);
      battery = constrain(batteryVoltage / 1024.0f, 0.0f, 1.0f);
    }

    /*if (gameIndex == GAME_UNKNOWN) {
      SSD1306* display = espert->oled.getDisplay();
      String string = String(round(batteryVoltage));
      display->setColor(color);
      display->drawString(x - 1 - display->getStringWidth(string), y - 3, string);
    }*/

    drawBitmap(x, y, 16, 8, batteryBitmap, NULL, color);

    for (int i = 0; i < (int)round(battery * 10.0f); i++) {
      if (battery > 0.2f || (battery <= 0.2f && isBlink())) {
        drawBitmap(x + 2 + i, y + 2, 8, 8, batteryIndicatorBitmap, NULL, color);
      }
    }
  }
}

void Game::renderFPS(int x, int y, int bitmapWidth, int bitmapHeight, int gap, const uint8_t* numberBitmap, const uint8_t* numberMaskBitmap, int color, int fpsBitmapX, int fpsBitmapY) {
  if (isFPSVisibled && numberBitmap) {
    if (fpsBitmapX > -1 && fpsBitmapY > -1) {
      drawBitmap(fpsBitmapX, fpsBitmapY, 16, 8, fpsBitmap, fpsMaskBitmap, color);
    }

    for (int i = 0; i < 2 && (fpsDigit[i] >= 0 && fpsDigit[i] <= 9); i++) {
      drawBitmap(x + (i * (bitmapWidth + gap)), y, bitmapWidth, bitmapHeight, numberBitmap + (fpsDigit[i] * bitmapWidth), numberMaskBitmap ? numberMaskBitmap + (fpsDigit[i] * bitmapWidth) : NULL, color);
    }
  }
}

void Game::renderVolume(int x, int y, int color) {
  if (isSoundEnabled) {
    int i = (int)round(volume * (numberOfVolumeFrames - 1));

    if (i == 0 && volume > 0.0f) {
      i = 1;
    }

    drawBitmap(x, y, 16, 8, volumeBitmap[i], NULL, color);
  }
}

void Game::renderMakerAsia(int x, int y, int color) {
  drawBitmap(x, y, 64, 16, makerAsiaBitmap, makerAsiaMaskBitmap, color);
}

void Game::update() {
}

void Game::resetGameTime() {
  lastFrameTime = millis();
  elapsedTime = 0.0f;
  blinkTime = 0.0f;
  fpsLastFrameTime = lastFrameTime;
}

void Game::updateGameTime(bool updateButton) {
  // game time
  unsigned long frameTime = millis();
  elapsedTime = frameTime - lastFrameTime;
  lastFrameTime = frameTime;

  // frame rate
  frameCount++;
  if (frameTime - fpsLastFrameTime >= 1000l) {
    if (frameRate != frameCount) {
      frameRate = frameCount;
      String fpsString = longToString(constrain(frameRate, 0, 99), 2, "0");

      for (int i = 0; i < 2; i++) {
        fpsDigit[i] = fpsString.charAt(i) - '0';
      }
    }

    frameCount = 0l;
    fpsLastFrameTime = frameTime;
  }

  // blink
  blinkTime += elapsedTime;
  if (blinkTime >= maxBlinkTime) {
    blinkTime = 0.0f;
  }

  // button
  if (updateButton) {
    buttonDelay += elapsedTime;
    if (buttonDelay >= maxButtonDelay) {
      buttonDelay = 0.0f;
      pressButton();
    }
  }

  // sound
  if (isSoundEnabled) {
    if (soundDuration > 0.0f) {
      soundDuration -= elapsedTime;
      if (soundDuration <= 0.0f) {
        soundDuration = 0.0f;
        espert->buzzer.off();
        isSoundInterruptEnabled = true;
      }
    } else if (nextSoundDelay > 0.0f) {
      nextSoundDelay -= elapsedTime;
      if (nextSoundDelay <= 0.0f) {
        nextSoundDelay = 0.0f;
        playSound(nextSound);
      }
    }
  }
}

void Game::writeHighScore(int offset) {
  if (gameIndex != GAME_UNKNOWN) {
    highScoreAddress = getHighScoreAddress();

    if (highScoreAddress != -1) {
      espert->eeprom.write(highScoreAddress + offset, longToString(highScore, String(maxScore[gameIndex]).length(), "0"));
    }
  }
}

void Game::writeVolume() {
  espert->eeprom.write(saveDataKey.length(), floatToString(volume, volumeLength, 2));
}
