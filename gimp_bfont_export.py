#! /usr/bin/python
# -*- coding: utf-8 -*-
import platform
import struct
from gimpfu import *


def export_to_bfont(image, drawable, filename, rawfilename, dummy1, endline_type):
    glyph_layer_found = False
    for layer in image.layers:
        if layer.name == "Glyphs":
            glyph_layer_found = True
            break

    if not glyph_layer_found:
        gimp.pdb.gimp_message("No layer named 'Glyphs' found.\n"
                              "Are you sure you have used\n"
                              "File -> Create -> Render font as bitmap...?")
        gimp.quit()

    para = image.parasite_find("bfont-charset")
    if para is None:
        gimp.pdb.gimp_message("Font data not found in the image.\n"
                              "Are you sure you have used\n"
                              "File -> Create -> Render font as bitmap...?")
        gimp.quit()

    rgn_glyphs = layer.get_pixel_rgn(0, 0,  image.width,  image.height)

    # --- Compose character dictionary (and read pixels + convert to 1 bpp / 2 bpp) ---

    font_name = "Unknown"
    #font_size = 0.0
    font_height = 0
    char_count = 0

    char_dict = []  # We use a list despite the name
    one_char_data = []  # UTF-8 character code (1 to 4 bytes), character offset (bytes from the start)
    char_x = 0
    char_y = 0
    char_width = 0
    # Fixed part (font header) length =
    # byte: number of bits per pixel
    # + word: character height (pixels)
    # + dword: number of characters
    # ---------------------------------
    # Total: 7 bytes
    char_dict_len = 7
    char_count = 0

    char_bytes = []  # Actual glyph 1 bpp (or more than 1 bpp) data
    # Parse data string from the parasite, left for us by font renderer
    # (gimp_bfont_render.py). Look for the uses of parasite_str above the call
    # to img.attach_new_parasite for string format, it's pretty self-explanatory.
    split_data = para.data.split('\n')  # Separator used in the parasite string
    data_len = len(split_data)
    for i, data_el in enumerate(split_data):
        if i == 0:
            font_name = data_el
        elif i == 1:
            font_height = int(data_el)
        elif i == 2:
            char_count = int(data_el)
        elif i == 3:
            bits_per_pixel = int(data_el)
        else:  # Character dictionary
            #print "i =", i
            if (i - 4) % 4 == 0:  # We start with i == 4
                one_char_data = [data_el]  # UTF-8 character
                char_dict_len += len(data_el)
            elif (i - 5) % 4 == 0:
                char_x = int(data_el)
            elif (i - 6) % 4 == 0:
                char_y = int(data_el)
            elif (i - 7) % 4 == 0:
                char_width = int(data_el)
                one_char_data.append(char_width)
                char_offset = len(char_bytes)

                # And fill character data array while we're at it
                empty_lines_top, empty_lines_bottom = \
                    read_char_pixels(char_bytes, rgn_glyphs, font_height,
                                     char_x, char_y, char_width, bits_per_pixel)
                #print "empty_lines_top =", empty_lines_top, ", empty_lines_bottom =", empty_lines_bottom
                #print "0x{:02X}, 0x{:02X}".format(empty_lines_top, empty_lines_bottom)

                one_char_data.append(empty_lines_top)
                one_char_data.append(empty_lines_bottom)
                if empty_lines_top < font_height:
                    one_char_data.append(char_offset)
                else:
                    one_char_data.append(0)  # Empty glyph - no bitmap data available

                # Offset value is going to be updated by adding the total length
                # of char_dict (in bytes, obviously).
                #print "one_char_data =", one_char_data
                char_dict.append(one_char_data)
                # Increase character dictionary length, taking into account -
                # word: character width
                # + byte: number of empty lines at the top
                # + byte: number of empty lines at the bottom
                # + dword: character offset (bytes from the start)
                # ------------------------------------------------
                # Total: 8 bytes
                char_dict_len += 8

        gimp.progress_update(i / (data_len * 2.0))

    prog = float(i) / (data_len + len(char_dict))
    gimp.progress_update(prog)

    # Correct character offset values by adding correct character dictionary length to them.
    # Character dictionary length is unknown before it is finished, so it was not taken into account.
    for cd_el in char_dict:
        cd_el[4] += char_dict_len

    # --- Export as C array ---

    # Platform-specific EOL bytes (<CR><LF> (10, 13) for Windows, <LF> for others, like Linux or OS X)
    if endline_type == "os":
        if platform.system().upper().startswith("WINDOWS"):
            newline = "\r\n"
        else:
            newline = "\n"
    elif endline_type == "crlf":
        newline = "\r\n"
    else:
        newline = "\n"

    out_file = open(filename, "wb")

    out_file.write("// Little-endian byte order is used;" + newline)
    out_file.write("// Byte means 8 bits, word means 16 bits, dword means 32 bits" + newline + newline)

    out_file.write("// Font header:" + newline)
    out_file.write("//     byte: number of bits per pixel. Currently only 1 bpp and 2 bpp are supported" + newline)
    out_file.write("//     word: character height (pixels)" + newline)
    out_file.write("//     dword: number of characters" + newline)
    out_file.write("//     Character dictionary: one or more of" + newline)
    out_file.write("//         Character description:" + newline)
    out_file.write("//             byte..dword: UTF-8 character code (1 to 4 bytes, as specified by UTF-8. see https://en.wikipedia.org/wiki/UTF-8#Description )" + newline)
    out_file.write("//             word: character width" + newline)
    out_file.write("//             byte: number of empty lines (lines without foreground pixels - background only) at the top of the character glyph" + newline)
    out_file.write("//             byte: number of empty lines (lines without foreground pixels - background only) at the bottom of the character glyph" + newline)
    out_file.write("//             dword: character offset (bytes from the start)" + newline + newline)

    out_file.write("// Characters: one or more of" + newline)
    out_file.write("//     Character:" + newline)
    out_file.write("//         (character height) minus (number of empty lines at the top) minus (number of empty lines at the bottom) lines:" + newline)
    out_file.write("//             Each line is as many bits as needed to describe (character width) pixels," + newline)
    out_file.write("//             1 or 2 bits per pixel, least significant bit is pixel #0 (leftmost pixel)." + newline)
    out_file.write("//             Each line starts on the next bit after the previous line." + newline)
    out_file.write("const uint8_t {}{}[] = ".format(font_name.lower().replace(' ', '_').replace('-', '_'), font_height) + newline +
                   "{" + newline +
                   "    0x{:02X},"  # Bits per pixel
                   " 0x{:02X},0x{:02X},"  # word: character height (pixels)
                   " 0x{:02X},0x{:02X},0x{:02X},0x{:02X},"  # dword: number of characters
                   " // byte: bits per pixel; word: character height (pixels);"
                   " dword: number of characters".format(
                                            bits_per_pixel & 0x00FF,
                                            font_height & 0x00FF,
                                            (font_height & 0xFF00) >> 8,
                                            char_count & 0x000000FF,
                                            (char_count & 0x0000FF00) >> 8,
                                            (char_count & 0x00FF0000) >> 16,
                                            (char_count & 0xFF000000) >> 24) + newline)
    for cd_el in char_dict:
        #print "cd_el =", cd_el
        out_file.write("    ")
        # byte..dword:	UTF-8 character code (1 to 4 bytes, as specified by UTF-8. see https://en.wikipedia.org/wiki/UTF-8#Description )
        for utf8_byte in cd_el[0]:
            out_file.write("0x{:02X},".format(ord(utf8_byte)))
        out_file.write(" 0x{:02X},0x{:02X},"  # word: character width
                       " 0x{:02X}, 0x{:02X},"  # byte: number of empty lines at the top; byte: number of empty lines at the bottom
                       " 0x{:02X},0x{:02X},0x{:02X},0x{:02X},"  # dword: character offset (bytes from the start)
                       " // '{}'".format(cd_el[1] & 0x00FF,
                                         (cd_el[1] & 0xFF00) >> 8,  # character width
                                         cd_el[2], cd_el[3],  # Number of empty lines at the top and at the bottom of the glyph
                                         cd_el[4] & 0x000000FF,  # character offset
                                         (cd_el[4] & 0x0000FF00) >> 8,
                                         (cd_el[4] & 0x00FF0000) >> 16,
                                         (cd_el[4] & 0xFF000000) >> 24,
                                         cd_el[0]) + newline)

    prev_cd_el = None
    for cd_idx in xrange(0, len(char_dict)):
        out_file.write("    ")
        #print "--- '{}': {} px wide, {} px tall ---".format(char_dict[cd_idx][0], char_dict[cd_idx][1], font_height)
        char_end_offset = char_dict[cd_idx + 1][4] - char_dict_len if cd_idx < len(char_dict) - 1 \
                          else len(char_bytes)
        #print "--- from byte {} to {} ---".format(char_dict[cd_idx][2] - char_dict_len, char_end_offset - 1)
        for i in xrange(char_dict[cd_idx][4] - char_dict_len, char_end_offset):
            #print "Byte {} - 0x{:02X}".format(i, char_bytes[i])
            out_file.write("0x{:02X}".format(char_bytes[i]))
            if i < len(char_bytes) - 1:
                out_file.write(",")

        out_file.write(" // '{}' {} px wide".format(char_dict[cd_idx][0],
                                                    char_dict[cd_idx][1]) + newline)
        prog += 1.0 / len(char_dict)
        gimp.progress_update(prog)

    out_file.write("};")

    out_file.close()


def read_char_pixels(char_bytes, rgn_glyphs, font_height,
                     char_x, char_y, char_width, bits_per_pixel):
    """
    Converts glyph pixels to bits_per_pixel (1 and 2 are currently supported)
    and packs them into bytes, which are added to char_bytes.
    Empty lines at the top and bottom of the glyph
    are ignored (actually they are counted and those numbers are returned).
    Returns:
        number of empty lines on the top of the glyph,
        number of empty lines on the bottom of the glyph.
    """
    #transparent_black_pixel = struct.pack("BBBB", 0, 0, 0, 0)  # The color to ignore

    # Find out how many empty lines at the bottom first
    empty_lines_top = \
        count_empty_top_lines(rgn_glyphs, font_height, char_x, char_y,
                              char_width)

    if empty_lines_top == font_height:  # Empty glyph
        return empty_lines_top, 0  # They're all at the top then

    empty_lines_bottom = \
        count_empty_bottom_lines(rgn_glyphs, font_height, char_x, char_y,
                                 char_width)
    data_byte = 0
    bit_index = 0

    shade_step = 256.0 / pow(2.0, bits_per_pixel)  # We only really need it if bits_per_pixel > 1

    for y in xrange(char_y + empty_lines_top, char_y + font_height - empty_lines_bottom):
        for x in xrange(char_x, char_x + char_width):
            pxl_color = struct.unpack("BBBB", rgn_glyphs[x, y])
            if pxl_color[3] != 0:  # Non-transparent
                if bits_per_pixel == 1:
                    data_byte |= 1 << bit_index
                else:
                    data_byte |= (int(float(pxl_color[3]) / shade_step)) << bit_index

            bit_index += bits_per_pixel
            if bit_index >= 8:
                char_bytes.append(data_byte)
                data_byte = 0
                bit_index = 0

    if bit_index > 0:
        char_bytes.append(data_byte)  # Last byte of this character

    return empty_lines_top, empty_lines_bottom


def count_empty_top_lines(rgn_glyphs, font_height, char_x, char_y, char_width):
    """
    Count and return the number of empty lines at the top of the glyph.
    It's a separate function because Python makes breaking out of nested loops a big PITA :E
    """
    for y in xrange(char_y, char_y + font_height):
        if y - char_y >= 0xFF:  # Can't exceed 1 byte
            return 0xFF
        for x in xrange(char_x, char_x + char_width):
            pxl_color = struct.unpack("BBBB", rgn_glyphs[x, y])
            #if rgn_glyphs[x, y] == target_pixel:
            if pxl_color[3] != 0:  # Non-transparent
                return y - char_y
    return font_height  # They're all empty


def count_empty_bottom_lines(rgn_glyphs, font_height, char_x, char_y, char_width):
    """
    Count and return the number of empty lines at the bottom of the glyph.
    It's a separate function because Python makes breaking out of nested loops a big PITA :E
    """
    for y in xrange(char_y + font_height - 1, char_y - 1, -1):
        if char_y + font_height - y - 1 >= 0xFF:  # Can't exceed 1 byte
            return 0xFF
        for x in xrange(char_x, char_x + char_width):
            pxl_color = struct.unpack("BBBB", rgn_glyphs[x, y])
            #if rgn_glyphs[x, y] == target_pixel:
            if pxl_color[3] != 0:  # Non-transparent
                return char_y + font_height - y - 1
    return font_height  # They're all empty


def register_save():
    gimp.register_save_handler("file-bfont-export", "bfont", "")


register("file-bfont-export",
         "Export bitmap font rendered with File/Create/Render font as bitmap...\r\n"
         "as a 1 or 2 bit per pixel C array.\r\n"
         "Any fully transparent pixel in Glyphs layer is exported as 0,\r\n"
         "all other pixels are exported as 1's (in 1-BPP mode), or as 1's,\r\n"
         "2's or 3's (in 2-BPP mode), according to their opacity.",
         "Export bitmap font as a C array",
         "Alexei Kireev",
         "Copyright 2017-2021 Alexei Kireev ( https://www.dekart.com )",
         "2021-01-08",
         "<Save>/Bitmap font files",
         "RGBA",
         [
             (PF_BOOL, "dummy1", "Blahblahblah", True),  # I don't know why, but the first parameter is ignored
             (PF_RADIO, "endline_type", "New line:", "os", (("OS default", "os"), ("CR+LF (Windows)", "crlf"), ("LF (Linux / Mac)", "lf")))
         ],
         [],
         export_to_bfont,
         on_query=register_save)

main()
