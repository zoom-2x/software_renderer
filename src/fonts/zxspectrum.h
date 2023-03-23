// ----------------------------------------------------------------------------------
// -- File: gcsr_zxspectrum_text.h
// ----------------------------------------------------------------------------------
// -- Author: Gabi C.
// -- Description:
// -- Created: 2022-11-19 20:53:58
// -- Modified: 2022-11-19 20:53:59
// ----------------------------------------------------------------------------------

#ifndef GCSR_ZXSPECTRUM_H
#define GCSR_ZXSPECTRUM_H

typedef __ALIGN__ struct
{
    u16 data[8];
} zxspectrum_char_t;

__ALIGN__ zxspectrum_char_t zxspectrum_font[] = {
    {
        // space
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // !
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // "
        0b0000000000000000,
        0b0000000000100100,
        0b0000000000100100,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // #
        0b0000000000000000,
        0b0000000000100100,
        0b0000000001111110,
        0b0000000000100100,
        0b0000000000100100,
        0b0000000001111110,
        0b0000000000100100,
        0b0000000000000000
    },

    {
        // $
        0b0000000000000000,
        0b0000000000001000,
        0b0000000000111110,
        0b0000000000101000,
        0b0000000000111110,
        0b0000000000001010,
        0b0000000000111110,
        0b0000000000001000
    },

    {
        // %
        0b0000000000000000,
        0b0000000001100010,
        0b0000000001100100,
        0b0000000000001000,
        0b0000000000010000,
        0b0000000000100110,
        0b0000000001000110,
        0b0000000000001000
    },

    {
        // &
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000101000,
        0b0000000000010000,
        0b0000000000101010,
        0b0000000001000100,
        0b0000000000111010,
        0b0000000000001000
    },

    {
        // '
        0b0000000000000000,
        0b0000000000001000,
        0b0000000000010000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // (
        0b0000000000000000,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000000100,
        0b0000000000000000
    },

    {
        // )
        0b0000000000000000,
        0b0000000000100000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000100000,
        0b0000000000000000
    },

    {
        // *
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010100,
        0b0000000000001000,
        0b0000000000111110,
        0b0000000000001000,
        0b0000000000010100,
        0b0000000000000000
    },

    {
        // +
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000111110,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000000000
    },

    {
        // ,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000100000
    },

    {
        // -
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111110,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // .
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000011000,
        0b0000000000011000,
        0b0000000000000000
    },

    {
        // /
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000010,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000011000,
        0b0000000000011000,
        0b0000000000000000
    },

    {
        // 0
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000110,
        0b0000000001001010,
        0b0000000001010010,
        0b0000000001100010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // 1
        0b0000000000000000,
        0b0000000000011000,
        0b0000000000101000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000111110,
        0b0000000000000000
    },

    {
        // 2
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000000000010,
        0b0000000000111100,
        0b0000000001000000,
        0b0000000001111110,
        0b0000000000000000
    },

    {
        // 3
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000000001110,
        0b0000000000000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // 4
        0b0000000000000000,
        0b0000000000001000,
        0b0000000000011000,
        0b0000000000101000,
        0b0000000001001000,
        0b0000000001111110,
        0b0000000000001000,
        0b0000000000000000
    },

    {
        // 5
        0b0000000000000000,
        0b0000000001111110,
        0b0000000001000000,
        0b0000000001111100,
        0b0000000000000010,
        0b0000000001000010,
        0b0000000000111110,
        0b0000000000000000
    },

    {
        // 6
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000000,
        0b0000000001111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // 7
        0b0000000000000000,
        0b0000000001111110,
        0b0000000000000010,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000010010,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // 8
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // 9
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000111110,
        0b0000000000000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // :
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // ;
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000100000
    },

    {
        // <
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000010000,
        0b0000000000001000,
        0b0000000000000100,
        0b0000000000000000
    },

    {
        // =
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111110,
        0b0000000000000000,
        0b0000000000111110,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // >
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000001000,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // ?
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000000000,
        0b0000000000001000,
        0b0000000000000000
    },

    {
        // @
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001001010,
        0b0000000001010110,
        0b0000000001011110,
        0b0000000001000000,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // A
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001111110,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // B
        0b0000000000000000,
        0b0000000001111100,
        0b0000000001000010,
        0b0000000001111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001111100,
        0b0000000000000000
    },

    {
        // C
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // D
        0b0000000000000000,
        0b0000000001111000,
        0b0000000001000100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000100,
        0b0000000001111000,
        0b0000000000000000
    },

    {
        // E
        0b0000000000000000,
        0b0000000001111110,
        0b0000000001000000,
        0b0000000001111100,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001111110,
        0b0000000000000000
    },

    {
        // F
        0b0000000000000000,
        0b0000000001111110,
        0b0000000001000000,
        0b0000000001111100,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000000000000
    },

    {
        // G
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000000,
        0b0000000001001110,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // H
        0b0000000000000000,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001111110,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // I
        0b0000000000000000,
        0b0000000000111110,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000111110,
        0b0000000000000000
    },

    {
        // J
        0b0000000000000000,
        0b0000000000000010,
        0b0000000000000010,
        0b0000000000000010,
        0b0000000000000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // K
        0b0000000000000000,
        0b0000000001000100,
        0b0000000001001000,
        0b0000000001110000,
        0b0000000001001000,
        0b0000000001000100,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // L
        0b0000000000000000,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001111110,
        0b0000000000000000
    },

    {
        // M
        0b0000000000000000,
        0b0000000001000010,
        0b0000000001100110,
        0b0000000001011010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // N
        0b0000000000000000,
        0b0000000001000010,
        0b0000000001100010,
        0b0000000001010010,
        0b0000000001001010,
        0b0000000001000110,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // O
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // P
        0b0000000000000000,
        0b0000000001111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001111100,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000000000000
    },

    {
        // Q
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001010010,
        0b0000000001001010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // R
        0b0000000000000000,
        0b0000000001111100,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001111100,
        0b0000000001000100,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // S
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000000,
        0b0000000000111100,
        0b0000000000000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // T
        0b0000000000000000,
        0b0000000011111110,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // U
        0b0000000000000000,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // V
        0b0000000000000000,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000000100100,
        0b0000000000011000,
        0b0000000000000000
    },

    {
        // W
        0b0000000000000000,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001000010,
        0b0000000001011010,
        0b0000000000100100,
        0b0000000000000000
    },

    {
        // X
        0b0000000000000000,
        0b0000000001000010,
        0b0000000000100100,
        0b0000000000011000,
        0b0000000000011000,
        0b0000000000100100,
        0b0000000001000010,
        0b0000000000000000
    },

    {
        // Y
        0b0000000000000000,
        0b0000000010000010,
        0b0000000001000100,
        0b0000000000101000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // Z
        0b0000000000000000,
        0b0000000001111110,
        0b0000000000000100,
        0b0000000000001000,
        0b0000000000010000,
        0b0000000000100000,
        0b0000000001111110,
        0b0000000000000000
    },

    {
        // [
        0b0000000000000000,
        0b0000000000001110,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001110,
        0b0000000000000000
    },

    {
        // "\" backslash
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001000000,
        0b0000000000100000,
        0b0000000000010000,
        0b0000000000001000,
        0b0000000000000100,
        0b0000000000000000
    },

    {
        // ]
        0b0000000000000000,
        0b0000000001110000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000001110000,
        0b0000000000000000
    },

    {
        // ^
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000101000,
        0b0000000001000100,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // _
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000011111111
    },

    {
        // `
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000001000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },

    {
        // a
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111000,
        0b0000000000000100,
        0b0000000000111100,
        0b0000000001000100,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // b
        0b0000000000000000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000111100,
        0b0000000000100010,
        0b0000000000100010,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // c
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000011100,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000011100,
        0b0000000000000000
    },

    {
        // d
        0b0000000000000000,
        0b0000000000000100,
        0b0000000000000100,
        0b0000000000111100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // e
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111000,
        0b0000000001000100,
        0b0000000001111000,
        0b0000000001000000,
        0b0000000000111100,
        0b0000000000000000
    },

    {
        // f
        0b0000000000000000,
        0b0000000000001100,
        0b0000000000010000,
        0b0000000000011000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // g
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000111100,
        0b0000000000000100,
        0b0000000000111000
    },

    {
        // h
        0b0000000000000000,
        0b0000000001000000,
        0b0000000001000000,
        0b0000000001111000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000000000
    },

    {
        // i
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000000000,
        0b0000000000110000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000111000,
        0b0000000000000000
    },

    {
        // j
        0b0000000000000000,
        0b0000000000000100,
        0b0000000000000000,
        0b0000000000000100,
        0b0000000000000100,
        0b0000000000000100,
        0b0000000000100100,
        0b0000000000011000
    },

    {
        // k
        0b0000000000000000,
        0b0000000000100000,
        0b0000000000101000,
        0b0000000000110000,
        0b0000000000110000,
        0b0000000000101000,
        0b0000000000100100,
        0b0000000000000000
    },

    {
        // l
        0b0000000000000000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000011000,
        0b0000000000000000
    },

    {
        // m
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001101000,
        0b0000000001010100,
        0b0000000001010100,
        0b0000000001010100,
        0b0000000001010100,
        0b0000000000000000
    },

    {
        // n
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001111000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000000000
    },

    {
        // o
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000111000,
        0b0000000000000000
    },

    {
        // p
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001111000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001111000,
        0b0000000001000000,
        0b0000000001000000
    },

    {
        // q
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000111100,
        0b0000000000000100,
        0b0000000000000110
    },

    {
        // r
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000011100,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000100000,
        0b0000000000000000
    },

    {
        // s
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000111000,
        0b0000000001000000,
        0b0000000000111000,
        0b0000000000000100,
        0b0000000001111000,
        0b0000000000000000
    },

    {
        // t
        0b0000000000000000,
        0b0000000000010000,
        0b0000000000111000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000000001100,
        0b0000000000000000
    },

    {
        // u
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000111000,
        0b0000000000000000
    },

    {
        // v
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000101000,
        0b0000000000101000,
        0b0000000000010000,
        0b0000000000000000
    },

    {
        // w
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001000100,
        0b0000000001010100,
        0b0000000001010100,
        0b0000000001010100,
        0b0000000000101000,
        0b0000000000000000
    },

    {
        // x
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001000100,
        0b0000000000101000,
        0b0000000000010000,
        0b0000000000101000,
        0b0000000001000100,
        0b0000000000000000
    },

    {
        // y
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000001000100,
        0b0000000000111100,
        0b0000000000000100,
        0b0000000000111000
    },

    {
        // z
        0b0000000000000000,
        0b0000000000000000,
        0b0000000001111100,
        0b0000000000001000,
        0b0000000000010000,
        0b0000000000100000,
        0b0000000001111100,
        0b0000000000000000
    },

    {
        // {
        0b0000000000000000,
        0b0000000000001110,
        0b0000000000001000,
        0b0000000000110000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001110,
        0b0000000000000000
    },

    {
        // |
        0b0000000000000000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000001000,
        0b0000000000000000
    },

    {
        // }
        0b0000000000000000,
        0b0000000001110000,
        0b0000000000010000,
        0b0000000000001100,
        0b0000000000010000,
        0b0000000000010000,
        0b0000000001110000,
        0b0000000000000000
    },

    {
        // ~
        0b0000000000000000,
        0b0000000000010100,
        0b0000000000101000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000,
        0b0000000000000000
    },


    {
        // copyright
        0b0000000000111100,
        0b0000000001000010,
        0b0000000010011001,
        0b0000000010100001,
        0b0000000010100001,
        0b0000000010011001,
        0b0000000001000010,
        0b0000000000111100
    },
};

const u16 cmask[8] = {
    0b10000000,
    0b01000000,
    0b00100000,
    0b00010000,
    0b00001000,
    0b00000100,
    0b00000010,
    0b00000001,
};

#endif