#include <ruby.h>
#include <zlib.h>
#include <arpa/inet.h>
#include "dyn_arr.h"

#define PNG_HEADER "\x89PNG\r\n\x1a\n"
#define PNG_BYTES2UINT(char_ptr) (ntohl(*(uint32_t *)(char_ptr)))

#define APPLE_PNG_OK 0
#define APPLE_PNG_STREAM_ERROR Z_STREAM_ERROR
#define APPLE_PNG_DATA_ERROR Z_DATA_ERROR
#define APPLE_PNG_ZLIB_VERSION_ERROR Z_VERSION_ERROR
#define APPLE_PNG_NO_MEM_ERROR Z_MEM_ERROR

/* inflate from apple png file, don't expect headers */
static int png_inflate(unsigned char *data, uint32_t length, uint32_t width, uint32_t height, unsigned char **out_buff, uint32_t *out_uncompressed_size) {
    int error;
    z_stream inflate_strm = {0};

    *out_uncompressed_size = height + width * height * 4;
    *out_buff = malloc(sizeof(char) * (*out_uncompressed_size));
    if (*out_buff == 0) {
        return APPLE_PNG_NO_MEM_ERROR;
    }

    inflate_strm.data_type = Z_BINARY;
    inflate_strm.avail_in = length;
    inflate_strm.next_in = (Bytef *)data;
    inflate_strm.avail_out = *out_uncompressed_size;
    inflate_strm.next_out = (Bytef *)*out_buff;

    error = inflateInit2(&inflate_strm, -15);
    if (error != Z_OK) {
        free(*out_buff);
        return error;
    }
    error = inflate(&inflate_strm, Z_FINISH);
    if (error != Z_OK && error != Z_STREAM_END) {
        free (*out_buff);
        return error;
    }
    error = inflateEnd(&inflate_strm);
    if (error != Z_OK) {
        free(*out_buff);
        return error;
    }

    return APPLE_PNG_OK;
}

/* deflate for standard png file */
static int png_deflate(unsigned char *data, uint32_t length, unsigned char **out_buff, uint32_t *out_compressed_size) {
    int error;
    uint32_t sizeEstimate;
    z_stream deflate_strm = {0};
    deflate_strm.avail_in = length;
    deflate_strm.next_in = (Bytef *)data;

    error = deflateInit(&deflate_strm, Z_DEFAULT_COMPRESSION);
    if (error != Z_OK) {
        return error;
    }

    sizeEstimate = (uint32_t)deflateBound(&deflate_strm, length);
    *out_buff = malloc(sizeof(unsigned char) * sizeEstimate);
    if (*out_buff == 0) {
        free(*out_buff);
        return APPLE_PNG_NO_MEM_ERROR;
    }

    deflate_strm.avail_out = (uint32_t)sizeEstimate;
    deflate_strm.next_out = *out_buff;

    error = deflate(&deflate_strm, Z_FINISH);
    if (error != Z_OK && error != Z_STREAM_END) {
        free(*out_buff);
        return error;
    }
    error = deflateEnd(&deflate_strm);
    if (error != Z_OK) {
        free(*out_buff);
        return error;
    }

    *out_compressed_size = (uint32_t)deflate_strm.total_out;

    return APPLE_PNG_OK;
}

/* flip first and third color in uncompressed png pixel data */
static void flip_color_bytes(unsigned char *pixelData, uint32_t width, uint32_t height) {
    uint32_t x, y;
    size_t i = 0;

    for (y = 0; y < height; y++) {
        i += 1;
        for (x = 0; x < width; x++) {
            char tmp = pixelData[i];
            pixelData[i] = pixelData[i + 2];
            pixelData[i + 2] = tmp;
            i += 4;
        }
    }
}

/* calculate chunk checksum out of chunk type and chunk data */
static uint32_t png_crc32(const char *chunkType, const char *chunkData, uint32_t chunkLength) {
    unsigned long running_crc = crc32(0, 0, 0);
    running_crc = crc32(running_crc, (Bytef *)chunkType, 4);
    running_crc = crc32(running_crc, (Bytef *)chunkData, chunkLength);
    return (uint32_t)((running_crc + 0x100000000) % 0x100000000);
}

/* extract chunks from PNG data */
static int readPngChunks(VALUE self, const char *oldPNG, size_t oldPngLength, dyn_arr *newPNG) {
    uint32_t width = 0, height = 0;
    size_t cursor = 8;
    dyn_arr *applePngCompressedPixelData = dyn_arr_create(oldPngLength);
    if (applePngCompressedPixelData == 0) {
        return APPLE_PNG_NO_MEM_ERROR;
    }

    while (cursor < oldPngLength) {
        int breakLoop = 0;
        const char *chunkLength_raw = &oldPNG[cursor];
        uint32_t chunkLength = PNG_BYTES2UINT(chunkLength_raw);
        const char *chunkType = &oldPNG[cursor + 4];
        const char *chunkData = &oldPNG[cursor + 8];
        const char *chunkCRC_raw = &oldPNG[cursor + 8 + chunkLength];
        cursor += chunkLength + 12;

        if (strncmp(chunkType, "IHDR", 4) == 0) {
            /* extract dimensions from header */
            width = PNG_BYTES2UINT(&chunkData[0]);
            height = PNG_BYTES2UINT(&chunkData[4]);
            rb_funcall(self, rb_intern("width="), 1, INT2NUM(width));
            rb_funcall(self, rb_intern("height="), 1, INT2NUM(height));
        } else if (strncmp(chunkType, "IDAT", 4) == 0) {
            /* collect pixel data to process it once an IEND chunk appears */
            dyn_arr_append(applePngCompressedPixelData, chunkData, chunkLength);
            continue;
        } else if (strncmp(chunkType, "CgBI", 4) == 0) {
            /* don't write CgBI chunks to the output png  */
            continue;
        } else if (strncmp(chunkType, "IEND", 4) == 0) {
            /* all png data has been procssed, now flip the color bytes */
            unsigned char *decompressedPixelData, *standardPngCompressedPixelData;
            uint32_t compressed_size, uncompressed_size;
            uint32_t chunkCRC;
            uint32_t tmp_chunkLength, tmp_chunkCRC;
            int error;

            /* decompress, flip color bytes, then compress again */
            error = png_inflate((unsigned char *)applePngCompressedPixelData->arr, (uint32_t)applePngCompressedPixelData->used, width, height, &decompressedPixelData, &uncompressed_size);
            if (error != APPLE_PNG_OK) {
                dyn_arr_free(applePngCompressedPixelData);
                return error;
            }
            flip_color_bytes(decompressedPixelData, width, height);
            error = png_deflate(decompressedPixelData, uncompressed_size, &standardPngCompressedPixelData, &compressed_size);
            if (error != APPLE_PNG_OK) {
                dyn_arr_free(applePngCompressedPixelData);
                return error;
            }

            /* clean up temporary data structures */
            dyn_arr_free(applePngCompressedPixelData);
            free(decompressedPixelData);

            /* write the pixel data to the output PNG as IDAT chunk */
            chunkType = "IDAT";
            chunkLength = compressed_size;
            tmp_chunkLength = htonl(chunkLength);
            chunkLength_raw = (char*)(&tmp_chunkLength);
            chunkData = (char*)standardPngCompressedPixelData;
            chunkCRC = png_crc32(chunkType, chunkData, chunkLength);
            tmp_chunkCRC = htonl(chunkCRC);
            chunkCRC_raw = (char *)(&tmp_chunkCRC);

            /* we're done */
            breakLoop = 1;
        }

        /* write the chunk to the output png */
        dyn_arr_append(newPNG, chunkLength_raw, 4);
        dyn_arr_append(newPNG, chunkType, 4);
        dyn_arr_append(newPNG, chunkData, chunkLength);
        dyn_arr_append(newPNG, chunkCRC_raw, 4);

        if (breakLoop) {
            break;
        }
    }

    return APPLE_PNG_OK;
}

/* convert an apple png data string to a standard png data string */
static VALUE ApplePng_convert_apple_png(VALUE self, VALUE data) {
    int error;
    VALUE ret;
    const char *oldPNG = StringValuePtr(data);
    size_t oldPNG_length = RSTRING_LEN(data);

    dyn_arr *newPNG = dyn_arr_create(oldPNG_length);
    if (newPNG == 0) {
        rb_raise(rb_eNoMemError, "There was not enough memory to uncompress the PNG data.");
    }

    dyn_arr_append(newPNG, PNG_HEADER, 8);
    error = readPngChunks(self, oldPNG, oldPNG_length, newPNG);
    if (error != APPLE_PNG_OK) {
        dyn_arr_free(newPNG);
        switch (error) {
        case APPLE_PNG_STREAM_ERROR:
        case APPLE_PNG_DATA_ERROR:
            rb_raise(rb_eArgError, "Could not process the input data. Please make sure this is valid Apple PNG format data.");
        case APPLE_PNG_ZLIB_VERSION_ERROR:
            rb_raise(rb_eArgError, "Unexpected Zlib version encountered. The caller was expecting Zlib " ZLIB_VERSION ".");
        case APPLE_PNG_NO_MEM_ERROR:
            rb_raise(rb_eArgError, "Ran out of memory while processing the PNG data.");
        default:
            rb_raise(rb_eStandardError, "An unexpected error was encountered while processing the PNG data. Please make sure the input is valid Apple PNG format data.");
        }
    }

    ret = rb_str_new(newPNG->arr, newPNG->used);
    dyn_arr_free(newPNG);
    return ret;
}

/* this is a quick way to get the width and height from png data without converting the file */
static VALUE ApplePng_get_dimensions(VALUE self, VALUE data) {
    const char *oldPNG = StringValuePtr(data);
    size_t oldPNG_length = RSTRING_LEN(data);
    size_t cursor = 8;

    /* check whether this is actually a png file */
    if (strncmp(PNG_HEADER, oldPNG, 8) != 0) {
        rb_raise(rb_eArgError, "Input data is not a valid PNG file (missing the PNG magic bytes).");
    }

    while (cursor < oldPNG_length) {
        uint32_t chunkLength = ntohl(*(uint32_t *)(&oldPNG[cursor]));
        const char *chunkType = &oldPNG[cursor + 4];
        const char *chunkData = &oldPNG[cursor + 8];
        cursor += chunkLength + 12;

        if (strncmp(chunkType, "IHDR", 4) == 0) {
            uint32_t width = PNG_BYTES2UINT(&chunkData[0]);
            uint32_t height = PNG_BYTES2UINT(&chunkData[4]);
            rb_funcall(self, rb_intern("width="), 1, INT2NUM(width));
            rb_funcall(self, rb_intern("height="), 1, INT2NUM(height));
            return Qnil;
        }
    }

    rb_raise(rb_eArgError, "Input data is not a valid PNG file (missing IHDR chunk).");
}

void Init_apple_png(void) {
  VALUE klass = rb_define_class("ApplePng", rb_cObject);
  rb_define_protected_method(klass, "convert_apple_png", ApplePng_convert_apple_png, 1);
  rb_define_protected_method(klass, "get_dimensions", ApplePng_get_dimensions, 1);
}