#include <Arduino.h>
#include <M5Unified.h>
#include <ATOM_PRINTER.h>
#include <SPIFFS.h>
#include "rect.hpp"

#define FILE_PATH "/test.bin"
#define MAX_WIDTH 384
#define IMAGE_HEIGHT 567

#define FONT_WIDTH 48
#define FONT_HEIGHT 70
#define FULL_FONT_COUNT 2
#define HALF_FONT_COUNT 2
#define MAX_LENGTH (MAX_WIDTH / FONT_WIDTH)

#define CHARACTER_TOP_Y 58

ATOM_PRINTER printer;

uint8_t buffer[IMAGE_HEIGHT * MAX_WIDTH / 8]; // Buffer for the bitmap data
Rect bufferRect = Rect(MAX_WIDTH, IMAGE_HEIGHT);
uint8_t fontFullBitmaps[FULL_FONT_COUNT][FONT_HEIGHT * FONT_WIDTH / 8]; // Array to hold font bitmaps
uint8_t fontHalfBitmaps[HALF_FONT_COUNT][FONT_HEIGHT * FONT_WIDTH / 2 / 8];
Rect fontFullRect = Rect(FONT_WIDTH, FONT_HEIGHT);
Rect fontHalfRect = Rect(FONT_WIDTH / 2, FONT_HEIGHT);

int16_t messagePrefix[] = {55, 1917, 1366}; // の勇者
size_t messagePrefixLength = sizeof(messagePrefix) / sizeof(messagePrefix[0]);
uint8_t testImage[256 * 256];
uint8_t testImageWidth;
uint8_t testImageHeight;

bool isTestImageLoaded = false;

void writeBitmapToBuffer(Rect bitmapRect, uint8_t *bitmap, Rect bufferRect, uint8_t *buffer, Point offset = Point(0, 0))
{
  for (auto y = 0; y < bitmapRect.height; y++)
  {
    auto distY = y + offset.y;
    auto bufferRowOffset = distY * bufferRect.width;
    auto bufferPixelOffset = bufferRowOffset + offset.x;

    auto bitmapRowOffset = y * bitmapRect.width;

    memcpy(
        buffer + bufferPixelOffset / 8,
        bitmap + bitmapRowOffset / 8,
        bitmapRect.width / 8);
  }
}

void makeImage(int16_t *String, int length)
{
  memset(buffer + (CHARACTER_TOP_Y * MAX_WIDTH / 8), 0, MAX_WIDTH * FONT_HEIGHT / 8); // Clear the buffer for the current row
  auto stringMaxLength = MAX_LENGTH - messagePrefixLength - 1;                        //-1は括弧の分
  if (length > stringMaxLength)
  {
    length = stringMaxLength;
  }

  auto xOffset = (MAX_WIDTH - length + messagePrefixLength + 1) / 2;                                          // 中央揃え 1は括弧
  writeBitmapToBuffer(fontHalfRect, fontHalfBitmaps[0], bufferRect, buffer, Point(xOffset, CHARACTER_TOP_Y)); // 【
  xOffset += FONT_WIDTH / 2;                                                                                  // 半分の幅を加算
  for (int i = 0; i < length; i++)
  {
    auto c = String[i];
    if (c >= 0 && c < FULL_FONT_COUNT)
    {
      auto offset = Point(i * FONT_WIDTH + xOffset, CHARACTER_TOP_Y);
      writeBitmapToBuffer(fontFullRect, fontFullBitmaps[c], bufferRect, buffer, offset);
    }
  }
  writeBitmapToBuffer(fontHalfRect, fontHalfBitmaps[1], bufferRect, buffer, Point(xOffset + length * FONT_WIDTH, CHARACTER_TOP_Y)); // 】
}

void setup()
{
  M5.begin();
  SPIFFS.begin();
  Serial.begin(115200);
  printer.begin(&Serial2);

  auto testFile = SPIFFS.open("/test.bin");
  testImageWidth = testFile.read();
  testImageHeight = testFile.read();
  auto readLen = testFile.read((uint8_t *)testImage, sizeof(testImage));
  testFile.close();
  auto needReadLen = (testImageWidth * testImageHeight) / 8;
  if (readLen >= needReadLen)
  {
    Serial.printf("Read test image: %d bytes, expected: %d bytes\n", readLen, needReadLen);
    isTestImageLoaded = true;
  }
  else
  {
    Serial.printf("Error reading test image, read %d bytes but expected %d bytes\n", readLen, needReadLen);
  }

  auto fullFontFile = SPIFFS.open("/test2.bin");

  readLen = fullFontFile.read((uint8_t *)fontFullBitmaps, sizeof(fontFullBitmaps));

  if (readLen == sizeof(fontFullBitmaps))
  {
    Serial.println("Full font bitmaps loaded successfully.");
  }
  else
  {
    Serial.printf("Error reading full font bitmaps, read %d bytes\n", readLen);
  }

  fullFontFile.close();
  auto halfFontFile = SPIFFS.open("/half.bin");
  readLen = halfFontFile.read((uint8_t *)fontHalfBitmaps, sizeof(fontHalfBitmaps));
  if (readLen == sizeof(fontHalfBitmaps))
  {
    Serial.println("Half font bitmaps loaded successfully.");
  }
  else
  {
    Serial.printf("Error reading half font bitmaps, read %d bytes\n", readLen);
  }

  Serial2.print("\x1b##QPIX");
  Serial2.write(6);

  while (!Serial2 || !Serial) // Wait for Serial to be ready
  {
    delay(100);
  }
  printer.init();
  Serial.println("done");
}

void loop()
{
  M5.update();
  if (Serial.available())
  {
    int16_t input[10];
    auto readLen = Serial.readBytesUntil('\n', (char *)input, sizeof(input));
    for (auto i = 0; i < readLen; i++)
    {
      uint8_t testBuf[(MAX_WIDTH * FONT_HEIGHT) / 8];
      Rect testRect = Rect(MAX_WIDTH, FONT_HEIGHT);
      int cIndex = input[i];
      if (cIndex >= 0 && cIndex < FULL_FONT_COUNT)
      {
        auto offset = Point(i * FONT_WIDTH, 0);
        writeBitmapToBuffer(fontFullRect, fontFullBitmaps[cIndex], testRect, testBuf, offset);
      }
      else
      {
        Serial.println("Invalid character: " + String(cIndex));
      }
    }
    printer.printBMP(0, MAX_WIDTH, FONT_HEIGHT, (uint8_t *)buffer);
  }
  if (M5.BtnA.wasPressed() && isTestImageLoaded)
  {
    auto testbufferRect = Rect(256, 256);
    writeBitmapToBuffer(Rect(testImageWidth, testImageHeight), testImage, testbufferRect, buffer);
    printer.printBMP(0, 256, 256, buffer);
  }
  delay(50);
}
