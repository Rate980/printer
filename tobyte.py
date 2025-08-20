import gi

gi.require_version("Gimp", "3.0")

from gi.repository import Gegl, GLib


def image2bytes(image):
    tmp = Gimp.Layer.new_from_visible(image, image, "tmp")
    width = tmp.get_width()
    height = tmp.get_height()
    rect = Gegl.Rectangle.new(0, 0, width, height)
    buf = tmp.get_buffer()
    pixels = buf.get(rect, 1.0, None, Gegl.AbyssPolicy.WHITE)
    ret = bytearray()
    index = 0
    byte_index = 0
    now_byte = 0
    while index < len(pixels):
        now_byte = (now_byte << 1) | (not (pixels[index]) & 1)
        byte_index += 1
        if byte_index == 8:
            ret.append(now_byte)
            byte_index = 0
            now_byte = 0
        index += 2
    return bytes(ret)


def image2testdata(image):
    ret = bytearray()
    ret.append(image.get_width().to_bytes(1))
    ret.append(image.get_height().to_bytes(1))
    ret.extend(image2bytes(image))
    return bytes(ret)


import csv
import time

with open(
    r"D:\Ratei\Documents\PlatformIO\Projects\printer\code.csv", encoding="utf8"
) as f:
    reader = csv.reader(f)
    data = [x for x in reader][0]

image = Gimp.get_images()[0]
text = image.get_layers()[0]


with open("full.bin", "wb") as f:
    for character in data:
        text.set_text(character)
        f.write(image2bytes(image))
        time.sleep(1 / 1000)  # Sleep for 10ms to avoid Gimp freezing
