#! /usr/bin/python
# -*- coding: utf-8 -*-
import struct
from gimpfu import *


def render_font_as_bitmap(font_name, font_size, bits_per_pixel, charset):
    charset = charset.decode("utf-8")
    charset_len = len(charset)
    #print type(charset)
    #print charset

    text_ext = pdb.gimp_text_get_extents_fontname(charset, font_size, PIXELS, font_name)
    # text_ext = (width, height, ascent, descent)
    img_width, img_height, text_ascent, text_descent = text_ext
    #print "Text extent =", text_ext
    # --- gimp_text_get_extents_fontname() lies to us, basically,
    # --- so we have to add extra space, otherwise we'll run out of canvas
    img_width += int(0.12 * charset_len * font_size)  # A carefully crafted formula :)
    # We'll remove everything to the right of the last glyph when we're done
    # rendering.

    #img_height = int(font_size)
    background_color = (255, 255, 255)

    pdb.gimp_context_push()  # Save current background and foreground colors
    gimp.set_background(background_color)

    img = gimp.Image(img_width, img_height, RGB)
    img.disable_undo()

    layer_markings = gimp.Layer(img, "Background", img_width, img_height,
                                RGBA_IMAGE, 100, NORMAL_MODE)
    layer_markings.fill(BACKGROUND_FILL)
    img.add_layer(layer_markings, 1)
    pdb.gimp_edit_fill(layer_markings, BACKGROUND_FILL)

    layer_glyphs = gimp.Layer(img, "Glyphs", img_width, img_height,
                              RGBA_IMAGE, 100, NORMAL_MODE)
    #layer_glyphs.fill(BACKGROUND_FILL)
    img.add_layer(layer_glyphs, 0)
    #pdb.gimp_edit_fill(layer_glyphs, BACKGROUND_FILL)


    rgn_markings = layer_markings.get_pixel_rgn(0, 0, img_width, img_height)
    rgn_glyphs = layer_glyphs.get_pixel_rgn(0, 0, img_width, img_height)
    """
    r = int(0)
    g = b = a = int(255)
    colors = struct.pack("BBBB", r, g, b, a) * 1 * img_height
    rgn[width:(width + 1), 0:img_height] = colors

    colors = struct.pack("BBBB", r, g, b, a) * 1 * width
    rgn[0:width, (img_height - 1):img_height] = colors
    #color = struct.pack("BBBB", r, g, b, a)
    #rgn[width, 0] = color

    # gimp-text-fontname(image IMAGE, drawable DRAWABLE (-1 for a new text layer),
    #                    float x, float y, string text, int32 border (>= -1),
    #                    int32 antialias (TRUE or FALSE), float size (text size in pixels or points),
    #                    int32 size-type (PIXELS (0), POINTS (1)), string fontname)
    floattext = pdb.gimp_text_fontname(img, layer_glyphs, 0, descent, charset, 0, 0, font_size,
                                       PIXELS, font_name)
    pdb.gimp_floating_sel_anchor(floattext)
    """

    # Save data about font name, height, charset rendered and character widths in a
    # parasite for the exporter (gimp_bfont_export.py) to read. Parasites keep data as strings, so we
    # must format our data accordingly. We're using \n as a separator.
    char_x = 0
    i = 0.0
    parasite_str_2 = ""
    # Always render black characters
    foreground_color = (0, 0, 0)
    gimp.set_foreground(foreground_color)
    for char in charset:
        char_width, char_height, char_ascent, char_descent = \
            render_char(img, img_width, layer_glyphs, rgn_glyphs, rgn_markings, char, font_name,
                        font_size, bits_per_pixel, char_x, 0)
        parasite_str_2 += u"\n{}\n{}\n{}\n{}".format(char, char_x, 0, char_width)
        char_x += char_width

        prgrs = i / charset_len
        gimp.progress_update(prgrs)
        i += 1.0

    #img.resize(char_x, img_height, 0, 0)  # Remove extra whitespace - leaves yellow-black frame
    """
    img.crop(char_x, img_height, 0, 0)  # Remove extra whitespace
    #print "Cropped image to", char_x, ",", img_height
    """
    # Here we assume that all characters are of the same height (as they should be since it's the same font)
    parasite_str = u"{}\n{}\n{}\n{}".format(font_name, char_height, charset_len, bits_per_pixel)  #font_size, charset_len, bits_per_pixel)
    parasite_str += parasite_str_2
    img.crop(char_x, char_height, 0, 0)  # Remove extra whitespace (img_height is somehow > char_height in Windows)
    #print "Cropped image to", char_x, ",", char_height

    charset_parasite = img.attach_new_parasite("bfont-charset", 1, parasite_str)
    # The second argument (flags) is is_persistent

    img.enable_undo()

    gimp.Display(img)  # Create a new window for our image
    gimp.displays_flush()  # Show the new window

    pdb.gimp_context_pop()  # Restore original background and foreground colors


def render_char(img, img_width, layer_glyphs, rgn_glyphs, rgn_markings, one_char,
                font_name, font_size, bits_per_pixel, x, y):
    """one_char is a UTF-8 character to render"""
    # --- Crutches ---
    width_correction = 0
    # Cedilla Ţţ and comma Țț seem to be mixed up in Roboto fonts.
    # Replace (proper) comma versions with (bad) cedilla verisions, so that
    # proper (comma) glyphs are rendered.
    if font_name.startswith("Roboto"):
        if one_char == u'Ț':
            one_char = u'Ţ'
        elif one_char == u'ț':
            one_char = u'ţ'
        """
        elif one_char == 'Ї':
            if font_size <= 16.0:
                width_correction = 1
            elif font_size <= 34.0:
                width_correction = 2
            elif font_size <= 56.0:
                width_correction = 3
            elif font_size <= 70.0:
                width_correction = 4
            elif font_size <= 88.0:
                width_correction = 5
            else:
                width_correction = 6
        elif one_char == 'ї':
            if font_size <= 14.0:
                width_correction = 1
            elif font_size <= 24.0:
                width_correction = 2
            elif font_size <= 34.0:
                width_correction = 3
            elif font_size <= 46.0:
                width_correction = 4
            elif font_size <= 58.0:
                width_correction = 5
            elif font_size <= 74.0:
                width_correction = 6
            elif font_size <= 78.0:
                width_correction = 7
            elif font_size <= 94.0:
                width_correction = 8
            else:
                width_correction = 9
        """

    char_ext = pdb.gimp_text_get_extents_fontname(one_char, font_size, PIXELS, font_name)
    char_width, char_height, char_ascent, char_descent = char_ext
    #    char_width += width_correction

    # gimp-text-fontname(image IMAGE, drawable DRAWABLE (-1 for a new text layer),
    #                    float x, float y, string text, int32 border (>= -1),
    #                    int32 antialias (TRUE or FALSE), float size (text size in pixels or points),
    #                    int32 size-type (PIXELS (0), POINTS (1)), string fontname)
    #floattext = pdb.gimp_text_fontname(img, layer_glyphs, x, y,
    #                                   one_char, 0, 0, font_size, PIXELS, font_name)
    floattext = pdb.gimp_text_fontname(img, layer_glyphs, x, y,
                                       one_char, 0, FALSE if bits_per_pixel == "1" else TRUE,
                                       font_size, PIXELS, font_name)  # TRUE = Antialiasing
    if floattext is not None:  # It is None for the last character, don't know why
        #img.floating_selection  # The floating selection layer, or None if there is no floating selection.
        pdb.gimp_floating_sel_anchor(floattext)
        # Try locating real width of the character by probing its pixels
        max_x = x + char_width - 1
        #transparent_black_pixel = struct.pack("BBBB", 0, 0, 0, 0)

        for pix_y in xrange(y, y + char_height):
            for pix_x in xrange(x + char_width, min(x + char_width + 10, img_width)):
                #print "{}:{} = {}".format(pix_x, pix_y, rgn_glyphs[pix_x, pix_y])
                #if rgn_glyphs[pix_x, pix_y] != transparent_black_pixel and \
                pxl_color = struct.unpack("BBBB", rgn_glyphs[pix_x, pix_y])
                if pxl_color[3] != 0 and \
                   pix_x > max_x:
                   #print "Bingo!\nOld max_x = {}".format(max_x)
                   #print "Black pixel found at {}, {}".format(pix_x, pix_y)
                   max_x = pix_x
        #print char_width, max_x
        if max_x - x + 1 > char_width:
            char_width = max_x - x + 1

    # Add cyan lines marking the right and bottom boundaries of the glyph
    r = int(0)
    g = b = a = int(255)

    colors = struct.pack("BBBB", r, g, b, a) * 1 * int(char_height)
    rgn_markings[(x + char_width - 1):(x + char_width), y:(y + int(char_height))] = colors

    colors = struct.pack("BBBB", r, g, b, a) * 1 * char_width
    rgn_markings[x:(x + char_width), (y + int(char_height) - 1):(y + int(char_height))] = colors

    return char_width, char_height, char_ascent, char_descent  # We might have corrected char_width


register("bfont-render",
         "Renders True Type font as a bitmap for export as a C array.\r\n"
         "Any fully transparent pixel in Glyphs layer is exported as 0,\r\n"
         "all other pixels are exported as 1's (in 1-BPP mode), or as 1's,\r\n"
         "2's or 3's (in 2-BPP mode), according to their opacity.\r\n"
         "Use Export or Export As, select Bitmap font (.bfont) as file type.",
         "Renders True Type font as a bitmap for export as a C array",
         "Alexei Kireev",
         "Copyright 2017-2021 Alexei Kireev ( https://www.dekart.com )",
         "2021-01-08",
         "_Render font as bitmap...",
         "",  # No open image is required for us to run
         [
             (PF_FONT, "font_name", "Font", "Monospace"),
             #(PF_SLIDER, "font_size", "Font size", 24, (2, 128, 1)),
             (PF_FLOAT, "font_size", "Font size", 24.0),
             (PF_RADIO, "bits_per_pixel", "Bits per pixel:", "1", (("1 (no antialiasing)", "1"), ("2 (4 shades)", "2"))),
             (PF_STRING, "charset", "Characters",
                         u" !\"#$%&'()*+,-./0123456789:;<=>?@"
                         u"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`"
                         u"abcdefghijklmnopqrstuvwxyz{|}~"
                         u"©«®»"
                         u"ÂÎâîĂăȘșȚț"
                         u"АБВГҐДЕЄЁЖЗИІЇЙКЛМНОПРСТУЎФХЦЧШЩЪЫЬЭЮЯ"
                         u"абвгґдеєёжзиіїйклмнопрстуўфхцчшщъыьэюя")
         ],
         [],
         render_font_as_bitmap, menu="<Image>/File/Create")

main()

"""
                         Leftover characters

                         ¡¢£¤¥¦§©«­®°±´·»¼½¾¿
                         ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞß
                         àáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿ
                         ĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğ
                         ĠġĢģĤĥĦħĨĩĪīĬĭĮįİıĲĳĴĵĶķĸĹĺĻļĽľĿ
                         ŀŁłŃńŅņŇňŉŊŋŌōŎŏŐőŒœŔŕŖŗŘřŚśŜŝŞş
                         ŠšŢţŤťŦŧŨũŪūŬŭŮůŰűŲųŴŵŶŷŸŹźŻżŽžſ
                         ƀƁƂƃƄƅƆƇƈƉƊƋƌƍƎƏƐƑƒƓƔƕƖƗƘƙƚƛƜƝƞƟ
"""
