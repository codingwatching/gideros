/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; -*-
* Copyright (c) 2011 Marcus Geelnard
*
* This file is part of CrunchMe.
*
* CrunchMe is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CrunchMe is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CrunchMe.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstring>
#include <iostream>
#include <cmath>
#include "png.h"
namespace zlib {
  #include <zlib.h>
}

using namespace std;

// Constructor
png::png()
{
    // Initialize the CRC table
    for (unsigned int n = 0; n < 256; ++n)
    {
        unsigned int c = n;
        for (int k = 0; k < 8; ++k)
        {
            if (c & 1)
                c = 0xedb88320 ^ (c >> 1);
            else
                c >>= 1;
        }
        mCRCTable[n] = c;
    }
}

// Calculate the CRC32 for a given data buffer
unsigned int png::calcCRC32(const char *data, unsigned int size)
{
    unsigned int c = 0xffffffff;
    for (unsigned int n = 0; n < size; ++n)
    {
        c = mCRCTable[(c ^ data[n]) & 0xff] ^ (c >> 8);
    }

    return c ^ 0xffffffff;
}

// Append a new CHUNK to the buffer
unsigned int png::appendChunk(char *in, unsigned int inSize, char *out,
    const char *chunkName)
{
    // Length
    *out++ = inSize >> 24;
    *out++ = inSize >> 16;
    *out++ = inSize >> 8;
    *out++ = inSize;

    // Chunk type
    const char *crcStart = out;
    *out++ = *chunkName++;
    *out++ = *chunkName++;
    *out++ = *chunkName++;
    *out++ = *chunkName++;

    // Data
    for (unsigned int i = 0; i < inSize; ++i)
        *out++ = *in++;

    // CRC
    unsigned int crc32 = calcCRC32(crcStart, inSize + 4);
    *out++ = crc32 >> 24;
    *out++ = crc32 >> 16;
    *out++ = crc32 >> 8;
    *out++ = crc32;

    return inSize + 12;
}


unsigned int png::deflate(const char *in, unsigned int inSize, char *out,
    unsigned int outSize)
{
    // Init z_stream object
    zlib::z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    int ret = zlib::deflateInit_(&zs, 9, ZLIB_VERSION, sizeof(zlib::z_stream));
    if (ret != Z_OK)
        return 0;

    // Set up buffer pointers
    zs.avail_in = inSize;
    zs.next_in = (zlib::Bytef*)in;
    zs.avail_out = outSize;
    zs.next_out = (zlib::Bytef*)out;

    // Deflate
    ret = zlib::deflate(&zs, Z_FINISH);
    if (ret == Z_STREAM_ERROR)
        return 0;

    // Return size of the compressed data
    return outSize - zs.avail_out;
}

static void calcBmpSize(unsigned int size, unsigned int &w, unsigned int &h)
{
	int pixels=(size+2)/3;
	w= sqrt(pixels)+1;
    //w = size < 4096 ? ((size+2)/3) : 4096;
    h = (size + w*3 - 1) / (w*3);
    if (w * h*3 < size) ++h;
}

// Compress a data stream as a PNG image
unsigned int png::compress(const char *in, unsigned int inSize, char *out,
    unsigned int outSize)
{
    // Allocate memory for a raw "bitmap"
    unsigned int bmpWidth, bmpHeight;
    calcBmpSize(inSize, bmpWidth, bmpHeight);
    unsigned int bmpSize = (bmpWidth*3 + 1) * bmpHeight;
    char *bmp = new char[bmpSize];
    if (!bmp)
        return 0;

    // Put the input data into the "bitmap"
    char *src = (char*)in, *dst = bmp;
    unsigned int count = 0;
    for (unsigned int y = 0; y < bmpHeight; ++y)
    {
        *dst++ = 0;    // Filter type: 0 (none)
        for (unsigned int x = 0; x < (bmpWidth*3); ++x)   // Raw "pixel" data
        {
            if (count < inSize)
                *dst++ = *src++;
            else
                *dst++ = ' ';
            ++count;
        }
    }

    // Compress the "bitmap" using zlib
    unsigned int compressedSize = bmpSize + (bmpSize >> 7) + 20; // Worst case deflate result
    char *compressedData = new char[compressedSize];
    if (!compressedData)
    {
        delete[] bmp;
        return 0;
    }
    compressedSize = deflate(bmp, bmpSize, compressedData, compressedSize);

    // We're done with the raw "bitmap"
    delete[] bmp;

    // Construct IHDR (image header) data
    char ihdr[13] = {
        (char)(bmpWidth>>24),(char)(bmpWidth>>16),(char)(bmpWidth>>8),(char)bmpWidth,     // Width
        (char)(bmpHeight>>24),(char)(bmpHeight>>16),(char)(bmpHeight>>8),(char)bmpHeight, // Height
        8,                                      // Bit depth
        2,                                      // Color type (RGBA)
        0,                                      // Compression method (DEFLATE)
        0,                                      // Filter method (adaptive)
        0                                       // Interlace method (no interlace)
    };

    // Check if the output data array has space for the entire PNG file
    unsigned int totalPNGSize = 8                       // File header
                                + sizeof(ihdr) + 12     // IHDR chunk
                                + compressedSize + 12   // IDAT chunk
                                + 12;                   // IEND chunk
    if (totalPNGSize > outSize)
    {
        delete[] compressedData;
        return 0;
    }

    dst = out;

    // Output the file header
    static const char pngHead[] = {(char)137, 80, 78, 71, 13, 10, 26, 10};
    for (unsigned int i = 0; i < sizeof(pngHead); ++i)
        *dst++ = pngHead[i];

    // Output the IHDR chunk
    dst += appendChunk(ihdr, sizeof(ihdr), dst, "IHDR");

    // Output the IDAT chunk
    dst += appendChunk(compressedData, compressedSize, dst, "IDAT");

    // Output the IEND chunk
    dst += appendChunk(NULL, 0, dst, "IEND");

    delete[] compressedData;

    return (unsigned int)(dst - out);
}


unsigned int png::maxCompressedSize(unsigned int size)
{
    // Get bitmap size
    unsigned int w, h, bmpSize;
    calcBmpSize(size, w, h);
    bmpSize = (w*3 + 1) * h;

    // Take into account zlib growth and PNG format overhead
    return bmpSize + (bmpSize >> 7) + 70;
}

