//#include <stdio.h>   // printf()
#include <stdlib.h>  // free()
#include <stdint.h>  // uint8_t, uint16_t, uint32_t
#include <string.h>  // memcmp()
#include <stdbool.h> // bool, true, false

#include <SDL2/SDL.h>

// The fonts are located in bfont_user_fonts.c
// PLEASE READ THE LICENSING TERMS FOR THE FONTS CONTAINED IN bfont_user_fonts.c
extern const uint8_t ubuntu_bold_italic_derivative_bfont_user_28px_1bpp[];
extern const uint8_t ubuntu_bold_italic_derivative_bfont_user_28px_2bpp[];
extern const uint8_t ubuntu_derivative_bfont_user_41px_1bpp[];
extern const uint8_t ubuntu_derivative_bfont_user_41px_2bpp[];
extern const uint8_t ubuntu_mono_derivative_bfont_user_17px_2bpp[];

const uint8_t* g_pfontSmaller = ubuntu_bold_italic_derivative_bfont_user_28px_2bpp;
const uint8_t* g_pfontLarger = ubuntu_derivative_bfont_user_41px_2bpp;
// Font heights
uint16_t g_smallerFontHeight = 0;
uint16_t g_largerFontHeight = 0;
uint16_t g_monoFontHeight = 0;

uint8_t g_textIndex = 0; // Text sample to render
uint8_t g_bppIndex = 1; // Whether we use 1 bit per pixel font (0) or 2-bpp font (1)

const unsigned int SCREEN_WIDTH = 640;
const unsigned int SCREEN_HEIGHT = 480;

struct PixelCol
{
	uint8_t r, g, b;
};

void coloredCircle(uint16_t x, uint16_t y);

// This function contains the actual calls to string drawing
// functions, using bfont data arrays
void redrawScreen(void);

// Draws 0-terminated UTF-8 string pointed to by pStr, top left corner at coordinates (x, y), using font
// pointed to by pFont, background color is bgColor, character color is fgColor.
// Returns string width in pixels.
uint16_t drawString(uint16_t x, uint16_t y, uint8_t* pStr, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor);
// Return width, in pixels, of the string pStr pointes to.
// The font pFont points to is used.
uint16_t getStringWidth(uint8_t* pStr, const uint8_t* pFont);
// Same as drawString, but treats x as the center of the string, rather than its left boundary
uint16_t drawCenteredString(uint16_t x, uint16_t y, uint8_t* pStr, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor);
// Same as drawString, but treats x as the right boundary of the string, rather than its left boundary
uint16_t drawRightAlignedString(uint16_t x, uint16_t y, uint8_t* pStr, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor);
// Returns the width in pixels of the glyph which represents
// UTF-8 character pointed to by pChar in font pointed to by pFont,
// or 0 if character not found in the font
uint16_t getGlyphWidth(const uint8_t* pFont, uint8_t* pChar);
// Draws a single UTF-8 character. pChar points to the character (1..4 bytes), x and y are coordinates of
// top left corner, pFont points to the font array to be used, fgColor is the color of the character,
// bgColor is used to fill the gaps.
// Returns character width (in pixels).
uint16_t drawChar(uint16_t x, uint16_t y, uint8_t* pChar, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor);
uint8_t getUtf8CharLen(uint8_t* pChar);

void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, struct PixelCol clr);

struct PixelCol* g_pPixBuf = NULL;

// Colors for redrawScreen() function
struct PixelCol g_colorFg;
struct PixelCol g_colorBg;
struct PixelCol g_colorBlackFg;
struct PixelCol g_colorWhiteBg;
struct PixelCol g_colorLightGrayFg;
struct PixelCol g_colorDarkGrayBg;
//struct PixelCol g_colorLightGrayFg;
struct PixelCol g_colorBlueGreenBg;

struct PixelCol g_colorMouseClick;

// Widths of buttons for different texts
uint16_t g_textBtn1Width = 0;
uint16_t g_textBtn2Width = 0;

// Position and width of BPP buttons
uint16_t g_1BppBtnX0 = 0;
uint16_t g_1BppBtnWidth = 0;
uint16_t g_2BppBtnWidth = 0;

// Widths of buttons for color switching
uint16_t g_colorBtn1Width = 0;
uint16_t g_colorBtn2Width = 0;
uint16_t g_colorBtn3Width = 0;
uint16_t g_colorBtn4Width = 0;

int main(int argc, char* args[])
{
	// Initialize SDL
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		//printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	// The window we'll be rendering to
	SDL_Window* pWindow = SDL_CreateWindow("bfont use example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if(pWindow == NULL)
	{
		//printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return -2;
	}

	// The renderer represents the output device (usually graphics card)
	// Function Parameters:
	// window     the window where rendering is displayed
	// index      the index of the rendering driver to initialize, or -1 to initialize
	//            the first one supporting the requested flags
	// flags      0, or one or more SDL_RendererFlags OR'd together:
	// SDL_RENDERER_SOFTWARE       the renderer is a software fallback
	// SDL_RENDERER_ACCELERATED    the renderer uses hardware acceleration
	// SDL_RENDERER_PRESENTVSYNC   present is synchronized with the refresh rate
	// SDL_RENDERER_TARGETTEXTURE  the renderer supports rendering to texture
	// Note that providing no flags gives priority to available SDL_RENDERER_ACCELERATED renderers.
	SDL_Renderer* pRenderer = SDL_CreateRenderer(pWindow, -1, 0);
	//SDL_Texture* pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
	//Uint32* g_pPixBuf = malloc(sizeof(Uint32) * SCREEN_WIDTH * SCREEN_HEIGHT);
	// --- SDL_PIXELFORMAT_RGB888 is a four byte format with the first byte ignored (XXRRGGBB), ---
	// --- while SDL_PIXELFORMAT_RGB24 is a tightly packed three byte format (RRGGBB) ---
	SDL_Texture* pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);
	//g_pPixBuf = malloc(sizeof(uint32_t) * 3 * SCREEN_WIDTH * SCREEN_HEIGHT);
	g_pPixBuf = malloc(sizeof(struct PixelCol) * SCREEN_WIDTH * SCREEN_HEIGHT);
	//g_pPixBuf = malloc(sizeof(uint8_t) * 3 * SCREEN_WIDTH * SCREEN_HEIGHT);

	// Initialize forground and background colors for redrawScreen() function
	g_colorBlackFg.r = g_colorBlackFg.g = g_colorBlackFg.b = 0x00;;
	g_colorWhiteBg.r = g_colorWhiteBg.g = g_colorWhiteBg.b = 0xFF;;
	g_colorLightGrayFg.r = g_colorLightGrayFg.g = g_colorLightGrayFg.b = 0xBB;
	g_colorDarkGrayBg.r = g_colorDarkGrayBg.g = g_colorDarkGrayBg.b = 0x43;
	g_colorBlueGreenBg.r = 0x08;
	g_colorBlueGreenBg.g = 0x32;
	g_colorBlueGreenBg.b = 0x3D;
	g_colorFg = g_colorBlackFg; // g_colorLightGrayFg; //
	g_colorBg = g_colorWhiteBg; // g_colorBlueGreenBg; //

	g_colorMouseClick.r = 64;
	g_colorMouseClick.g = 144;
	g_colorMouseClick.b = 255;
    
	// Get font heights from their byte arrays
	g_smallerFontHeight = *((uint16_t*)(g_pfontSmaller + 1));
	g_largerFontHeight = *((uint16_t*)(g_pfontLarger + 1));
    g_monoFontHeight = *((uint16_t*)(ubuntu_mono_derivative_bfont_user_17px_2bpp + 1));

	redrawScreen();

	// Get the surface contained by the window
	SDL_Surface* pWinSurface = SDL_GetWindowSurface(pWindow);

	SDL_Event ev;
	bool quit = false;
	bool leftMouseButtonDown = false;
	while(!quit)
	{        
		//SDL_UpdateTexture(pTexture, NULL, g_pPixBuf, SCREEN_WIDTH * sizeof(uint32_t));
		SDL_UpdateTexture(pTexture, NULL, g_pPixBuf, SCREEN_WIDTH * sizeof(struct PixelCol));
		//SDL_UpdateTexture(pTexture, NULL, g_pPixBuf, SCREEN_WIDTH * sizeof(uint8_t) * 3);

		SDL_WaitEvent(&ev);

		switch(ev.type)
		{
		case SDL_QUIT:
			quit = true;
			break;

		case SDL_MOUSEBUTTONUP:
			if(ev.button.button == SDL_BUTTON_LEFT)
				leftMouseButtonDown = false;
			else
				if(ev.button.button == SDL_BUTTON_RIGHT)
					redrawScreen();
			break;

		case SDL_MOUSEBUTTONDOWN:
			if(ev.button.button == SDL_BUTTON_LEFT)
			{
				leftMouseButtonDown = true;
				if(ev.motion.y >= SCREEN_HEIGHT - g_largerFontHeight)
				{
					if(ev.motion.x < g_textBtn1Width)
						g_textIndex = 0;
					else if(ev.motion.x < g_textBtn1Width + g_textBtn2Width)
						g_textIndex = 1;
					else if(ev.motion.x >= g_1BppBtnX0 &&
					        ev.motion.x < g_1BppBtnX0 + g_1BppBtnWidth)
					{
						g_bppIndex = 0;
						// Switch to 1 BPP fonts
						g_pfontSmaller = ubuntu_bold_italic_derivative_bfont_user_28px_1bpp;
						g_pfontLarger = ubuntu_derivative_bfont_user_41px_1bpp;
						// Get font heights from their byte arrays
						g_smallerFontHeight = *((uint16_t*)(g_pfontSmaller + 1));
						g_largerFontHeight = *((uint16_t*)(g_pfontLarger + 1));
					}
					else if(ev.motion.x >= g_1BppBtnX0 + g_1BppBtnWidth &&
					        ev.motion.x < g_1BppBtnX0 + g_1BppBtnWidth + g_2BppBtnWidth)
					{
						g_bppIndex = 1;
						// Switch to 2 BPP fonts
						g_pfontSmaller = ubuntu_bold_italic_derivative_bfont_user_28px_2bpp;
						g_pfontLarger = ubuntu_derivative_bfont_user_41px_2bpp;
						// Get font heights from their byte arrays
						g_smallerFontHeight = *((uint16_t*)(g_pfontSmaller + 1));
						g_largerFontHeight = *((uint16_t*)(g_pfontLarger + 1));
					}
					else if(ev.motion.x >= SCREEN_WIDTH - g_colorBtn4Width)
					{
						// Color button 4 clicked
						g_colorFg = g_colorLightGrayFg;
						g_colorBg = g_colorBlueGreenBg;
					}
					else if(ev.motion.x >= SCREEN_WIDTH - (g_colorBtn3Width + g_colorBtn4Width))
					{
						// Color button 3 clicked
						g_colorFg = g_colorLightGrayFg;
						g_colorBg = g_colorDarkGrayBg;
					}
					else if(ev.motion.x >= SCREEN_WIDTH - (g_colorBtn2Width + g_colorBtn3Width + g_colorBtn4Width))
					{
						// Color button 2 clicked
						g_colorFg = g_colorBlackFg;
						g_colorBg = g_colorWhiteBg;
					}
					else if(ev.motion.x >= SCREEN_WIDTH - (g_colorBtn1Width + g_colorBtn2Width + g_colorBtn3Width + g_colorBtn4Width))
					{
						// Color button 1 clicked
						g_colorFg = g_colorWhiteBg;
						g_colorBg = g_colorBlackFg;
					}

					redrawScreen();
				}
			}
			break;

		case SDL_MOUSEMOTION:
			if(leftMouseButtonDown)
				coloredCircle(ev.motion.x, ev.motion.y);
			break;
		}

		SDL_RenderClear(pRenderer);
		SDL_RenderCopy(pRenderer, pTexture, NULL, NULL);
		SDL_RenderPresent(pRenderer);
	}

	free(g_pPixBuf);
	SDL_DestroyTexture(pTexture);
	SDL_DestroyRenderer(pRenderer);
	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return 0;
}

void coloredCircle(uint16_t x, uint16_t y)
{
	if(y >= 2)
		for(int16_t h = x - 1; h <= x + 1; h++)
			if(h >= 0 && h < SCREEN_WIDTH)
				g_pPixBuf[(y - 2) * SCREEN_WIDTH + h] = g_colorMouseClick;

	for(int16_t u = y - 1; u <= y + 1; u++)
		if(u >= 0 && u < SCREEN_HEIGHT)
			for(int16_t h = x - 2; h <= x + 2; h++)
				if(h >= 0 && h < SCREEN_WIDTH)
					g_pPixBuf[u * SCREEN_WIDTH + h] = g_colorMouseClick;

	if(y < SCREEN_HEIGHT - 2)
		for(int16_t h = x - 1; h <= x + 1; h++)
			if(h >= 0 && h < SCREEN_WIDTH)
				g_pPixBuf[(y + 2) * SCREEN_WIDTH + h] = g_colorMouseClick;
}

// This function contains the actual calls to string drawing
// functions, using bfont data arrays
void redrawScreen(void)
{
	for(uint32_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		g_pPixBuf[i] = g_colorBg;

	if(g_textIndex == 0)
	{
		drawString(0, 0, (uint8_t*)"The quick brown fox jumps over the lazy dog", g_pfontSmaller, g_colorFg, g_colorBg);
		drawCenteredString(SCREEN_WIDTH / 2, g_smallerFontHeight, (uint8_t*)"The quick brown fox jumps over the lazy dog", g_pfontSmaller, g_colorFg, g_colorBg);
		drawRightAlignedString(SCREEN_WIDTH, g_smallerFontHeight * 2, (uint8_t*)"The quick brown fox jumps over the lazy dog", g_pfontSmaller, g_colorFg, g_colorBg);

		drawString(0, g_smallerFontHeight * 3, (uint8_t*)"!\"#$%&'()*+,-./0123456789:;<=>?", g_pfontSmaller, g_colorFg, g_colorBg);

		drawString(0, 10 + g_smallerFontHeight * 4, (uint8_t*)"Brick quiz whangs jumpy veldt fox", g_pfontLarger, g_colorFg, g_colorBg);
		drawCenteredString(SCREEN_WIDTH / 2, 10 + g_smallerFontHeight * 4 + g_largerFontHeight, (uint8_t*)"Quick wafting zephyrs vex bold Jim", g_pfontLarger, g_colorFg, g_colorBg);
		drawRightAlignedString(SCREEN_WIDTH, 10 + g_smallerFontHeight * 4 + g_largerFontHeight * 2, (uint8_t*)"Sphinx of black quartz judge my vow", g_pfontLarger, g_colorFg, g_colorBg);
		//
		drawCenteredString(SCREEN_WIDTH / 2, 10 + g_smallerFontHeight * 4 + g_largerFontHeight * 3, (uint8_t*)"!\"#$%&'()*+,-./0123456789:;<=>?", g_pfontLarger, g_colorFg, g_colorBg);
	}
	else
	{
		drawString(0, 0, (uint8_t*)"У рудога вераб'я ў сховішчы пад", g_pfontSmaller, g_colorFg, g_colorBg);
		drawString(0, g_smallerFontHeight, (uint8_t*)"фатэлем ляжаць нейкія гаючыя зёлкі", g_pfontSmaller, g_colorFg, g_colorBg);
		drawCenteredString(SCREEN_WIDTH / 2, g_smallerFontHeight * 2, (uint8_t*)"У рудога вераб'я ў сховішчы пад", g_pfontSmaller, g_colorFg, g_colorBg);
		drawCenteredString(SCREEN_WIDTH / 2, g_smallerFontHeight * 3, (uint8_t*)"фатэлем ляжаць нейкія гаючыя зёлкі", g_pfontSmaller, g_colorFg, g_colorBg);
		drawRightAlignedString(SCREEN_WIDTH, g_smallerFontHeight * 4, (uint8_t*)"У рудога вераб'я ў сховішчы пад", g_pfontSmaller, g_colorFg, g_colorBg);
		drawRightAlignedString(SCREEN_WIDTH, g_smallerFontHeight * 5, (uint8_t*)"фатэлем ляжаць нейкія гаючыя зёлкі", g_pfontSmaller, g_colorFg, g_colorBg);

		drawString(0, g_smallerFontHeight * 6, (uint8_t*)"!\"#$%&'()*+,-./0123456789:;<=>?", g_pfontSmaller, g_colorFg, g_colorBg);

		drawString(0, 10 + g_smallerFontHeight * 7, (uint8_t*)"У рудога вераб'я ў сховішчы пад", g_pfontLarger, g_colorFg, g_colorBg);
		drawCenteredString(SCREEN_WIDTH / 2, 10 + g_smallerFontHeight * 7 + g_largerFontHeight, (uint8_t*)"фатэлем ляжаць нейкія гаючыя зёлкі", g_pfontLarger, g_colorFg, g_colorBg);
		drawRightAlignedString(SCREEN_WIDTH, 10 + g_smallerFontHeight * 7 + g_largerFontHeight * 2, (uint8_t*)"У рудога вераб'я ў сховішчы пад", g_pfontLarger, g_colorFg, g_colorBg);
		//
		drawCenteredString(SCREEN_WIDTH / 2, 10 + g_smallerFontHeight * 7 + g_largerFontHeight * 3, (uint8_t*)"!\"#$%&'()*+,-./0123456789:;<=>?", g_pfontLarger, g_colorFg, g_colorBg);
	}

	drawCenteredString(SCREEN_WIDTH / 2, SCREEN_HEIGHT - g_largerFontHeight - 4 - g_monoFontHeight,
	                   (uint8_t*)"Click the buttons below", ubuntu_mono_derivative_bfont_user_17px_2bpp, g_colorFg, g_colorBg);

	// -- Buttons --
	// Text sample buttons
	g_textBtn1Width = drawString(0, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)"  F  ", g_pfontLarger, g_colorFg, g_colorBg);
	g_textBtn2Width = drawString(g_textBtn1Width, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)"  Ў  ", g_pfontLarger, g_colorFg, g_colorBg);
	if(g_textIndex == 0)
		drawRect(0, SCREEN_HEIGHT - g_largerFontHeight, g_textBtn1Width, g_largerFontHeight, g_colorFg);
	else
		drawRect(g_textBtn1Width, SCREEN_HEIGHT - g_largerFontHeight, g_textBtn2Width, g_largerFontHeight, g_colorFg);

	// 1 / 2 bits per pixel font buttons
	g_1BppBtnX0 = g_textBtn1Width + g_textBtn2Width + 15;
	g_1BppBtnWidth = drawString(g_1BppBtnX0, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)" 1 bpp ", g_pfontLarger, g_colorFg, g_colorBg);
	g_2BppBtnWidth = drawString(g_1BppBtnX0 + g_1BppBtnWidth, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)" 2 bpp ", g_pfontLarger, g_colorFg, g_colorBg);
	if(g_bppIndex == 0)
		drawRect(g_1BppBtnX0, SCREEN_HEIGHT - g_largerFontHeight, g_1BppBtnWidth, g_largerFontHeight, g_colorFg);
	else
		drawRect(g_1BppBtnX0 + g_1BppBtnWidth, SCREEN_HEIGHT - g_largerFontHeight, g_2BppBtnWidth, g_largerFontHeight, g_colorFg);

	// Color buttons
	g_colorBtn4Width = drawRightAlignedString(SCREEN_WIDTH, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)"  A  ", g_pfontLarger, g_colorLightGrayFg, g_colorBlueGreenBg);
	g_colorBtn3Width = drawRightAlignedString(SCREEN_WIDTH - g_colorBtn4Width, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)"  A  ", g_pfontLarger, g_colorLightGrayFg, g_colorDarkGrayBg);
	g_colorBtn2Width = drawRightAlignedString(SCREEN_WIDTH - g_colorBtn4Width - g_colorBtn3Width, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)"  A  ", g_pfontLarger, g_colorBlackFg, g_colorWhiteBg);
	g_colorBtn1Width = drawRightAlignedString(SCREEN_WIDTH - g_colorBtn4Width - g_colorBtn3Width - g_colorBtn2Width, SCREEN_HEIGHT - g_largerFontHeight, (uint8_t*)"  A  ", g_pfontLarger, g_colorWhiteBg, g_colorBlackFg);
}

// Draws 0-terminated UTF-8 string pointed to by pStr, top left corner at coordinates (x, y), using font
// pointed to by pFont, background color is bgColor, character color is fgColor.
// Returns string width in pixels.
uint16_t drawString(uint16_t x, uint16_t y, uint8_t* pStr, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor)
{
	uint16_t curX = x;
	for(; *pStr != 0x00; pStr += getUtf8CharLen(pStr))
		curX += drawChar(curX, y, pStr, pFont, fgColor, bgColor);
	return curX - x;
}

// Return width, in pixels, of the string pStr pointes to.
// The font pFont points to is used.
uint16_t getStringWidth(uint8_t* pStr, const uint8_t* pFont)
{
	uint16_t strWidth = 0;
	for(; *pStr != 0x00; pStr += getUtf8CharLen(pStr))
		strWidth += getGlyphWidth(pFont, pStr);
	return strWidth;
}

// Same as drawString, but treats x as the center of the string, rather than its left boundary
uint16_t drawCenteredString(uint16_t x, uint16_t y, uint8_t* pStr, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor)
{
	return drawString(x - (getStringWidth(pStr, pFont) >> 1), y, pStr, pFont, fgColor, bgColor);
}

// Same as drawString, but treats x as the right boundary of the string, rather than its left boundary
uint16_t drawRightAlignedString(uint16_t x, uint16_t y, uint8_t* pStr, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor)
{
	return drawString(x - getStringWidth(pStr, pFont), y, pStr, pFont, fgColor, bgColor);
}

// Returns the width in pixels of the glyph which represents
// UTF-8 character pointed to by pChar in font pointed to by pFont,
// or 0 if character not found in the font
uint16_t getGlyphWidth(const uint8_t* pFont, uint8_t* pChar)
{
	// For documentation on offsets (e.g. pFont + 3), see font array definition
	uint32_t numChars = *((uint32_t*)(pFont + 3));
	// Look in the character dictionary of the font for our *pChar
	uint8_t charLen = getUtf8CharLen(pChar); // How many bytes in *pChar (1..4)
	uint8_t* pDictChar = (uint8_t*)pFont + 7; // Skip font header
	uint32_t i = 0; // Declare it here so that i remains visible after we leave for loop
	for(; i < numChars; i++)
	{
		uint8_t dictCharLen = getUtf8CharLen(pDictChar);
		if(dictCharLen == charLen)
			if(memcmp((const void*)pChar, (const void*)pDictChar, (size_t)charLen) == 0)
				break;
		pDictChar += dictCharLen + 8; // Next character
	}
	if(i >= numChars)
		return 0; // Character not found

	uint16_t charWidth = *((uint16_t*)(pDictChar + charLen));
	return charWidth;
}

// Draws a single UTF-8 character. pChar points to the character (1..4 bytes), x and y are coordinates of
// top left corner, pFont points to the font array to be used, fgColor is the color of the character,
// bgColor is used to fill the gaps.
// Returns character width (in pixels).
uint16_t drawChar(uint16_t x, uint16_t y, uint8_t* pChar, const uint8_t* pFont, struct PixelCol fgColor, struct PixelCol bgColor)
{
	// For documentation on offsets (e.g. pFont + 3), see font array definition
	uint8_t fontBpp = *pFont; // We really only support 1-BPP and 2-BPP fonts at the moment. Must be an even number of bits anyway.
	uint32_t numChars = *((uint32_t*)(pFont + 3));
	// Look in the character dictionary of the font for our *pChar
	uint8_t charLen = getUtf8CharLen(pChar); // How many bytes in *pChar (1..4)
	uint8_t* pDictChar = (uint8_t*)pFont + 7; // Skip font header
	uint32_t i = 0; // Declare it here so that i remains visible after we leave for loop
	for(; i < numChars; i++)
	{
		uint8_t dictCharLen = getUtf8CharLen(pDictChar);
		if(dictCharLen == charLen)
			if(memcmp((const void*)pChar, (const void*)pDictChar, (size_t)charLen) == 0)
				break;
		pDictChar += dictCharLen + 8; // Next character
	}
	if(i >= numChars)
		return 0; // Character not found

	// Character found
	uint16_t charHeight = *((uint16_t*)(pFont + 1));
	uint16_t charWidth = *((uint16_t*)(pDictChar + charLen));
	uint8_t numEmptyLinesTop = *(pDictChar + charLen + 2);
	uint8_t numEmptyLinesBottom = *(pDictChar + charLen + 3);

	// Colors for 2 BPP font - values 01 and 02 (00 is bgColor, 03 is fgColor)
	struct PixelCol colStep;
	colStep.r = (fgColor.r - bgColor.r) / 3;
	colStep.g = (fgColor.g - bgColor.g) / 3;
	colStep.b = (fgColor.b - bgColor.b) / 3;
	struct PixelCol color01;
	color01.r = bgColor.r + colStep.r;
	color01.g = bgColor.g + colStep.g;
	color01.b = bgColor.b + colStep.b;
	struct PixelCol color02;
	color02.r = color01.r + colStep.r;
	color02.g = color01.g + colStep.g;
	color02.b = color01.b + colStep.b;

	uint8_t* pCharData = (uint8_t*)pFont + *((uint32_t*)(pDictChar + charLen + 4)); // offset is stored as 4 bytes in the array

	// Top empty lines
	struct PixelCol* pPxl = g_pPixBuf + y * SCREEN_WIDTH + x;
    for(uint16_t pxY = 0; pxY < numEmptyLinesTop; pxY++)
	{
		for(uint16_t pxX = 0; pxX < charWidth; pxX++)
			*pPxl++ = bgColor;
		pPxl += (SCREEN_WIDTH - charWidth);
	}

	// Glyph proper
	uint8_t dataByte = *pCharData++; // New line always starts with a new byte 
	uint8_t bitNo = 0;
	for(uint16_t u = 0; u < charHeight - numEmptyLinesTop - numEmptyLinesBottom; u++)
	{
		for(uint16_t h = 0; h < charWidth; h++)
		{
			if(fontBpp == 2)
				switch(dataByte & 0x03)
				{
				case 0x00:
					*pPxl++ = bgColor;
					break;

				case 0x01:
					*pPxl++ = color01;
					break;

				case 0x02:
					*pPxl++ = color02;
					break;

				default: // 0x03
					*pPxl++ = fgColor;
				}
			else // assume fontBpp == 1
				if(dataByte & 0x01)
					*pPxl++ = fgColor;
				else
					*pPxl++ = bgColor;

			bitNo += fontBpp;
			if(bitNo >= 8)
			{
				dataByte = *pCharData++; // Next byte, please
				bitNo = 0;
			}
			else
				dataByte >>= fontBpp;
		}

		pPxl += (SCREEN_WIDTH - charWidth);
	}

	// Bottom empty lines
	for(uint16_t pxY = 0; pxY < numEmptyLinesBottom; pxY++)
	{
		for(uint16_t pxX = 0; pxX < charWidth; pxX++)
			*pPxl++ = bgColor;
		pPxl += (SCREEN_WIDTH - charWidth);
	}

	return charWidth;
}

uint8_t getUtf8CharLen(uint8_t* pChar)
{
	if((*pChar & 0x80) == 0)
		return 1;
	if((*pChar & 0xE0) == 0xC0)
		return 2;
	if((*pChar & 0xF0) == 0xE0)
		return 3;
	if((*pChar & 0xF8) == 0xF0)
		return 4;
	return 0; // Should never arrive here
}

void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, struct PixelCol clr)
{
	if(x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT)
		return;

	if(x + w >= SCREEN_WIDTH + 1)
		w = SCREEN_WIDTH - 1 - x;
	if(y + h >= SCREEN_HEIGHT + 1)
		h = SCREEN_HEIGHT - 1 - y;
	//printf("x = %u, y = %u, w = %u, h = %u\n", x, y, w, h);

	struct PixelCol* pPxlTop = g_pPixBuf + y * SCREEN_WIDTH + x;
	struct PixelCol* pPxlBtm = pPxlTop + (h - 1) * SCREEN_WIDTH;
	struct PixelCol* pPxlLft = pPxlTop + SCREEN_WIDTH;
	struct PixelCol* pPxlRt = pPxlLft + w - 1;
	// Horizontal lines
	for(uint16_t i = 0; i < w; i++)
		*pPxlTop++ = *pPxlBtm++ = clr;

	for(uint16_t i = 0; i < h - 2; i++)
	{
		*pPxlLft = *pPxlRt = clr;
		pPxlLft += SCREEN_WIDTH;
		pPxlRt += SCREEN_WIDTH;
	}
}
