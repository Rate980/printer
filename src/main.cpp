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

ATOM_PRINTER printer;

uint8_t buffer[IMAGE_HEIGHT * MAX_WIDTH / 8]; // Buffer for the bitmap data
Rect bufferRect = Rect(MAX_WIDTH, IMAGE_HEIGHT);
uint8_t fontFullBitmaps[FULL_FONT_COUNT][FONT_HEIGHT * FONT_WIDTH / 8]; // Array to hold font bitmaps
uint8_t fontHalfBitmaps[HALF_FONT_COUNT][FONT_HEIGHT * FONT_WIDTH / 2 / 8];
Rect fontFullRect = Rect(FONT_WIDTH, FONT_HEIGHT);
Rect fontHalfRect = Rect(FONT_WIDTH / 2, FONT_HEIGHT);

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
    auto input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > MAX_LENGTH)
    {
      input = input.substring(0, MAX_LENGTH);
    }
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < input.length(); i++)
    {
      auto c = input.charAt(i);
      int cIndex = String(c).toInt();
      if (cIndex >= 0 && cIndex < 5)
      {
        auto offset = Point(i * FONT_WIDTH, 0);
        writeBitmapToBuffer(fontFullRect, fontFullBitmaps[cIndex], bufferRect, buffer, offset);
      }
      else
      {
        Serial.println("Invalid character: " + String(c));
      }
    }
    printer.printBMP(0, MAX_WIDTH, FONT_HEIGHT, (uint8_t *)buffer);
  }
  if (M5.BtnA.wasPressed() && isTestImageLoaded)
  {
    printer.printBMP(0, testImageWidth, testImageHeight, testImage);
  }
  delay(50);
}
