#include <Arduino.h>
#include <M5Unified.h>
#include <ATOM_PRINTER.h>
#include <SPIFFS.h>

#define FILE_PATH "/test.bin"
#define MAX_WIDTH 384
#define IMAGE_HEIGHT 567

#define FONT_WIDTH 48
#define FONT_HEIGHT 70
#define MAX_LENGTH (MAX_WIDTH / FONT_WIDTH)

ATOM_PRINTER printer;

uint8_t buffer[IMAGE_HEIGHT][MAX_WIDTH / 8];             // Buffer for the bitmap data
uint8_t fontFullBitmaps[5][FONT_HEIGHT][FONT_WIDTH / 8]; // Array to hold font bitmaps
uint8_t fontHalfBitmaps[2][FONT_HEIGHT][FONT_WIDTH / 2 / 8];

void setup()
{
  M5.begin();
  SPIFFS.begin();
  Serial.begin(115200);
  printer.begin(&Serial2);

  for (int i = 0; i < 5; i++)
  {
    auto file = SPIFFS.open("/" + String(i) + ".bin", "r");
    if (file)
    {
      auto readLen = file.read((uint8_t *)fontFullBitmaps[i], sizeof(fontFullBitmaps[i]));
      if (readLen != sizeof(fontFullBitmaps[i]))
      {
        Serial.printf("Error reading font file %d, read %d bytes\n", i, readLen);
      }
      else
      {
        Serial.printf("Font file %d loaded successfully\n", i);
      }
    }
    else
    {
      Serial.printf("Failed to open font file %d\n", i);
    }

    // uint8_t cmd[] = {0x1b, 0x23, 0x23, 0x51, 0x50, 0x49, 0x58, 24};
    // Serial2.write(cmd, sizeof(cmd));
    Serial2.print("\x1b##QPIX");
    Serial2.write(6);
  }

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
        for (int j = 0; j < FONT_HEIGHT; j++)
        {
          memcpy(buffer[j] + (i * FONT_WIDTH / 8), fontFullBitmaps[cIndex][j], FONT_WIDTH / 8);
        }
      }
      else
      {
        Serial.println("Invalid character: " + String(c));
      }
    }
    printer.printBMP(0, MAX_WIDTH, FONT_WIDTH, (uint8_t *)buffer);
  }
  if (M5.BtnA.wasPressed())
  {
    printer.printPos(50);

    Serial2.write(0x1b);
    Serial2.print('a');
    Serial2.write(0x01);
    printer.printQRCode("https://example.com");
  }
  delay(50);
}
