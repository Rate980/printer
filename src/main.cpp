#include <Arduino.h>
#include <M5Unified.h>
#include <ATOM_PRINTER.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <base64.hpp>
#include "rect.hpp"

#define LED_PIN 27

#define FILE_PATH "/test.bin"
#define MAX_WIDTH 384
#define IMAGE_HEIGHT 570

#define FONT_WIDTH 48
#define FONT_HEIGHT 70
#define FULL_FONT_COUNT 3151
#define HALF_FONT_COUNT 2
#define MAX_LENGTH (MAX_WIDTH / FONT_WIDTH)
#define FILL_FONT_FILENAME "/full.bin"
#define FONT_SIZE (FONT_HEIGHT * FONT_WIDTH / 8)

#define CHARACTER_TOP_Y 50

#define SSID "maruyama"
#define PASSWORD "marufuck"

ATOM_PRINTER printer;
uint8_t buffer[IMAGE_HEIGHT * MAX_WIDTH / 8]; // Buffer for the bitmap data
Rect bufferRect = Rect(MAX_WIDTH, IMAGE_HEIGHT);
// uint8_t fontFullBitmaps[FULL_FONT_COUNT][FONT_HEIGHT * FONT_WIDTH / 8]; // Array to hold font bitmaps
uint8_t fontHalfBitmaps[HALF_FONT_COUNT][FONT_HEIGHT * FONT_WIDTH / 2 / 8];
Rect fontFullRect = Rect(FONT_WIDTH, FONT_HEIGHT);
Rect fontHalfRect = Rect(FONT_WIDTH / 2, FONT_HEIGHT);

uint16_t messagePrefix[] = {55, 2917, 1365}; // の勇者
size_t messagePrefixLength = sizeof(messagePrefix) / sizeof(messagePrefix[0]);

bool isTestImageLoaded = false;

void readBitmapFromFile(uint16_t index, uint8_t *buffer)
{
  auto file = SPIFFS.open(FILL_FONT_FILENAME, "r");
  file.seek(index * FONT_SIZE, SeekSet);
  auto readlen = file.read(buffer, FONT_SIZE);
  if (readlen != FONT_SIZE)
  {
    Serial.printf("Error reading bitmap from file, read %d bytes\n", readlen);
  }
  file.close();
}

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

void makeImage(uint16_t *String, int length)
{
  memset(buffer + (CHARACTER_TOP_Y * MAX_WIDTH / 8), 0, MAX_WIDTH * FONT_HEIGHT / 8); // Clear the buffer for the current row
  auto stringMaxLength = MAX_LENGTH - messagePrefixLength - 1;                        //-1は括弧の分
  if (length > stringMaxLength)
  {
    length = stringMaxLength;
  }

  auto xOffset = (MAX_WIDTH - (length + messagePrefixLength + 1) * FONT_WIDTH) / 2; // 中央揃え 1は括弧
  Serial.println(xOffset);
  writeBitmapToBuffer(fontHalfRect, fontHalfBitmaps[0], bufferRect, buffer, Point(xOffset, CHARACTER_TOP_Y)); // 【
  xOffset += FONT_WIDTH / 2;                                                                                  // 半分の幅を加算
  Serial.println(xOffset);
  uint8_t fontBuf[FONT_SIZE];
  uint16_t message[MAX_LENGTH];
  auto messageLength = length + messagePrefixLength;
  memcpy(message, String, length * sizeof(uint16_t));
  memcpy(message + length, messagePrefix, sizeof(messagePrefix));
  for (int i = 0; i < messageLength; i++)
  {
    auto c = message[i];
    if (c >= 0 && c < FULL_FONT_COUNT)
    {
      auto offset = Point(i * FONT_WIDTH + xOffset, CHARACTER_TOP_Y);
      readBitmapFromFile(c, fontBuf);

      writeBitmapToBuffer(fontFullRect, fontBuf, bufferRect, buffer, offset);
    }
  }
  writeBitmapToBuffer(fontHalfRect, fontHalfBitmaps[1], bufferRect, buffer, Point(xOffset + messageLength * FONT_WIDTH, CHARACTER_TOP_Y)); // 】
}

void setup()
{
  M5.begin();
  SPIFFS.begin(true);
  Serial.begin(115200);
  printer.begin(&Serial2);
  WiFi.begin(SSID, PASSWORD);

  size_t readLen;

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

  auto backFile = SPIFFS.open("/back.bin");
  readLen = backFile.read((uint8_t *)buffer, sizeof(buffer));
  backFile.close();

  while (!Serial2 || !Serial) // Wait for Serial to be ready
  {
    delay(100);
  }
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(100);
  // }

  // HTTPClient client;
  // client.begin("http://heroes.noonyuu.com/save_result/title_code");
  // auto status = client.GET();
  // if (status == HTTP_CODE_OK)
  // {
  //   Serial.println("Connected to server successfully.");
  //   String response = client.getString();
  //   uint16_t buf[4];
  //   auto decodedLength = decode_base64((unsigned char *)response.c_str(), response.length(), (unsigned char *)buf);
  //   for (auto i = 0; i < decodedLength / 2; i++)
  //   {
  //     Serial.printf("Decoded value %d: %d\n", i, buf[i]);
  //   }
  // }
  // else
  // {
  //   Serial.printf("Failed to connect to server, status: %d\n", status);
  // }

  // printer.init();
  Serial2.print("\x1b##QPIX");
  Serial2.write(6);
  Serial.println("done");
}

void loop()
{
  M5.update();
  if (Serial.available())
  {
    uint16_t input[MAX_LENGTH];
    auto readLen = Serial.readBytesUntil(0xFF, (char *)input, sizeof(input));
    uint8_t fontBuf[FONT_SIZE];
    uint8_t testBuf[(MAX_WIDTH * FONT_HEIGHT) / 8];
    Rect testRect = Rect(MAX_WIDTH, FONT_HEIGHT);
    for (auto i = 0; i < min(readLen / 2, (size_t)MAX_LENGTH); i++)
    {
      int cIndex = input[i];
      Serial.println(cIndex);
      if (cIndex >= 0 && cIndex < FULL_FONT_COUNT)
      {
        auto offset = Point(i * FONT_WIDTH, 0);
        readBitmapFromFile(cIndex, fontBuf);
        writeBitmapToBuffer(fontFullRect, fontBuf, testRect, testBuf, offset);
      }
      else
      {
        Serial.println("Invalid character: " + String(cIndex));
      }
    }
    printer.printBMP(0, MAX_WIDTH, FONT_HEIGHT, (uint8_t *)testBuf);
  }
  if (M5.BtnA.wasPressed() && WiFi.status() == WL_CONNECTED)
  {
    // printer.printBMP(0, testImageWidth, testImageHeight, testImage);
    uint16_t message[10];
    HTTPClient client;
    client.begin("http://heroes.noonyuu.com/save_result/title_code");
    auto status = client.GET();
    if (status == HTTP_CODE_OK)
    {
      auto resp = client.getString();
      auto decodedLength = decode_base64((unsigned char *)resp.c_str(), resp.length(), (unsigned char *)message);
      Serial.println("Message read successfully.");
      makeImage(message, decodedLength / 2);
      printer.printBMP(0, MAX_WIDTH, IMAGE_HEIGHT, buffer);
      Serial2.print("\033a");
      Serial2.write(1);
      printer.printQRCode("https://heroes.noonyuu.com/heroes");
      printer.newLine(10);
    }
    else
    {
      Serial.printf("Failed to read message, status: %d\n", status);
    }
    // uint8_t testBuf[FONT_SIZE];
    // readBitmapFromFile(2917, testBuf);
    // printer.printBMP(0, FONT_WIDTH, FONT_HEIGHT, testBuf);
  }
  delay(50);
}
