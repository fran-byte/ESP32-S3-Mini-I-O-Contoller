#pragma once
#include <U8g2lib.h>

// ======================== SimpleUnicode.h ========================
// Custom bitmap glyphs for symbols that don't render well with standard fonts
// Each glyph is 8x8 pixels, stored as 8 bytes (1 byte per row)
// Bit 1 = pixel ON, Bit 0 = pixel OFF
// MSB (bit 7) = leftmost pixel, LSB (bit 0) = rightmost pixel

namespace SimpleUnicode
{
    // ● - Filled Circle (for RUNNING status)
    // 8x8 bitmap
    const uint8_t FILLED_CIRCLE[8] = {
        0b00111100,  // ..####..
        0b01111110,  // .######.
        0b11111111,  // ########
        0b11111111,  // ########
        0b11111111,  // ########
        0b11111111,  // ########
        0b01111110,  // .######.
        0b00111100   // ..####..
    };

    // ○ - Empty Circle (for STOPPED status)
    // 8x8 bitmap
    const uint8_t EMPTY_CIRCLE[8] = {
        0b00111100,  // ..####..
        0b01000010,  // .#....#.
        0b10000001,  // #......#
        0b10000001,  // #......#
        0b10000001,  // #......#
        0b10000001,  // #......#
        0b01000010,  // .#....#.
        0b00111100   // ..####..
    };

    // → - Right Arrow (for CW direction)
    // 8x8 bitmap
    const uint8_t ARROW_RIGHT[8] = {
        0b00010000,  // ...#....
        0b00011000,  // ...##...
        0b00011100,  // ...###..
        0b11111110,  // #######.
        0b11111110,  // #######.
        0b00011100,  // ...###..
        0b00011000,  // ...##...
        0b00010000   // ...#....
    };

    // ← - Left Arrow (for CCW direction)
    // 8x8 bitmap
    const uint8_t ARROW_LEFT[8] = {
        0b00001000,  // ....#...
        0b00011000,  // ...##...
        0b00111000,  // ..###...
        0b01111111,  // .#######
        0b01111111,  // .#######
        0b00111000,  // ..###...
        0b00011000,  // ...##...
        0b00001000   // ....#...
    };

    // ↻ - Circular Arrow (for RPM indicator)
    // 8x8 bitmap
    const uint8_t ROTATE_ARROW[8] = {
        0b00111100,  // ..####..
        0b01000010,  // .#....#.
        0b10000111,  // #....###
        0b10000010,  // #.....#.
        0b10000010,  // #.....#.
        0b10000111,  // #....###
        0b01000010,  // .#....#.
        0b00111100   // ..####..
    };

    // ✓ - Check Mark (for OK/Good status)
    // 8x8 bitmap
    const uint8_t CHECK_MARK[8] = {
        0b00000000,  // ........
        0b00000001,  // .......#
        0b00000011,  // ......##
        0b10000110,  // #....##.
        0b11001100,  // ##..##..
        0b01111000,  // .####...
        0b00110000,  // ..##....
        0b00000000   // ........
    };

    // ✗ - X Mark (for ALARM/Bad status)
    // 8x8 bitmap
    const uint8_t X_MARK[8] = {
        0b10000001,  // #......#
        0b11000011,  // ##....##
        0b01100110,  // .##..##.
        0b00111100,  // ..####..
        0b00111100,  // ..####..
        0b01100110,  // .##..##.
        0b11000011,  // ##....##
        0b10000001   // #......#
    };

    // █ - Full Block (for progress bar filled)
    // 6x8 bitmap (narrower for better bar)
    const uint8_t BLOCK_FULL[8] = {
        0b11111100,  // ######..
        0b11111100,  // ######..
        0b11111100,  // ######..
        0b11111100,  // ######..
        0b11111100,  // ######..
        0b11111100,  // ######..
        0b11111100,  // ######..
        0b11111100   // ######..
    };

    // ░ - Light Block (for progress bar empty)
    // 6x8 bitmap (pattern)
    const uint8_t BLOCK_LIGHT[8] = {
        0b10100000,  // #.#.....
        0b00000000,  // ........
        0b10100000,  // #.#.....
        0b00000000,  // ........
        0b10100000,  // #.#.....
        0b00000000,  // ........
        0b10100000,  // #.#.....
        0b00000000   // ........
    };

    // Helper function to draw a custom glyph at position (x, y)
    void drawGlyph(U8G2 *u8g2, int x, int y, const uint8_t *bitmap)
    {
        u8g2->drawXBM(x, y, 8, 8, bitmap);
    }

    // Convenience functions for each symbol
    inline void drawFilledCircle(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, FILLED_CIRCLE); 
    }

    inline void drawEmptyCircle(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, EMPTY_CIRCLE); 
    }

    inline void drawArrowRight(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, ARROW_RIGHT); 
    }

    inline void drawArrowLeft(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, ARROW_LEFT); 
    }

    inline void drawRotateArrow(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, ROTATE_ARROW); 
    }

    inline void drawCheckMark(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, CHECK_MARK); 
    }

    inline void drawXMark(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, X_MARK); 
    }

    inline void drawBlockFull(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, BLOCK_FULL); 
    }

    inline void drawBlockLight(U8G2 *u8g2, int x, int y) 
    { 
        drawGlyph(u8g2, x, y, BLOCK_LIGHT); 
    }

    // Draw progress bar (horizontal)
    // x, y: top-left corner
    // length: number of blocks (total)
    // filled: number of filled blocks
    void drawProgressBar(U8G2 *u8g2, int x, int y, int length, int filled)
    {
        for (int i = 0; i < length; i++)
        {
            if (i < filled)
                drawBlockFull(u8g2, x + (i * 6), y);  // 6px width per block
            else
                drawBlockLight(u8g2, x + (i * 6), y);
        }
    }

} // namespace SimpleUnicode

/*
 * USAGE EXAMPLES:
 * 
 * #include "SimpleUnicode.h"
 * 
 * // In your draw function:
 * 
 * // Draw filled circle at (10, 10)
 * SimpleUnicode::drawFilledCircle(&u8g2, 10, 10);
 * 
 * // Draw right arrow at (50, 20)
 * SimpleUnicode::drawArrowRight(&u8g2, 50, 20);
 * 
 * // Draw progress bar: 14 blocks total, 7 filled
 * SimpleUnicode::drawProgressBar(&u8g2, 10, 30, 14, 7);
 * 
 * // Draw check mark at (100, 40)
 * SimpleUnicode::drawCheckMark(&u8g2, 100, 40);
 */
