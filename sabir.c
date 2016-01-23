#line 1 "imp.c"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>
#include <math.h>
#include <float.h>
#include <inttypes.h>

#line 1 "utf8proc.h"
/*
 * Copyright (c) 2015 Steven G. Johnson, Jiahao Chen, Peter Colberg, Tony Kelman, Scott P. Jones, and other contributors.
 * Copyright (c) 2009 Public Software Group e. V., Berlin, Germany
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/**
 * @mainpage
 *
 * utf8proc is a free/open-source (MIT/expat licensed) C library
 * providing Unicode normalization, case-folding, and other operations
 * for strings in the UTF-8 encoding, supporting Unicode version
 * 8.0.0.  See the utf8proc home page (http://julialang.org/utf8proc/)
 * for downloads and other information, or the source code on github
 * (https://github.com/JuliaLang/utf8proc).
 *
 * For the utf8proc API documentation, see: @ref utf8proc.h
 *
 * The features of utf8proc include:
 *
 * - Transformation of strings (@ref utf8proc_map) to:
 *    - decompose (@ref UTF8PROC_DECOMPOSE) or compose (@ref UTF8PROC_COMPOSE) Unicode combining characters (http://en.wikipedia.org/wiki/Combining_character)
 *    - canonicalize Unicode compatibility characters (@ref UTF8PROC_COMPAT)
 *    - strip "ignorable" (@ref UTF8PROC_IGNORE) characters, control characters (@ref UTF8PROC_STRIPCC), or combining characters such as accents (@ref UTF8PROC_STRIPMARK)
 *    - case-folding (@ref UTF8PROC_CASEFOLD)
 * - Unicode normalization: @ref utf8proc_NFD, @ref utf8proc_NFC, @ref utf8proc_NFKD, @ref utf8proc_NFKC
 * - Detecting grapheme boundaries (@ref utf8proc_grapheme_break and @ref UTF8PROC_CHARBOUND)
 * - Character-width computation: @ref utf8proc_charwidth
 * - Classification of characters by Unicode category: @ref utf8proc_category and @ref utf8proc_category_string
 * - Encode (@ref utf8proc_encode_char) and decode (@ref utf8proc_iterate) Unicode codepoints to/from UTF-8.
 */

/** @file */

#ifndef UTF8PROC_H
#define UTF8PROC_H

/** @name API version
 *
 * The utf8proc API version MAJOR.MINOR.PATCH, following
 * semantic-versioning rules (http://semver.org) based on API
 * compatibility.
 *
 * This is also returned at runtime by @ref utf8proc_version; however, the
 * runtime version may append a string like "-dev" to the version number
 * for prerelease versions.
 *
 * @note The shared-library version number in the Makefile may be different,
 *       being based on ABI compatibility rather than API compatibility.
 */
/** @{ */
/** The MAJOR version number (increased when backwards API compatibility is broken). */
#define UTF8PROC_VERSION_MAJOR 1
/** The MINOR version number (increased when new functionality is added in a backwards-compatible manner). */
#define UTF8PROC_VERSION_MINOR 3
/** The PATCH version (increased for fixes that do not change the API). */
#define UTF8PROC_VERSION_PATCH 0
/** @} */

#include <stdlib.h>
#include <sys/types.h>
#ifdef _MSC_VER
typedef signed char utf8proc_int8_t;
typedef unsigned char utf8proc_uint8_t;
typedef short utf8proc_int16_t;
typedef unsigned short utf8proc_uint16_t;
typedef int utf8proc_int32_t;
typedef unsigned int utf8proc_uint32_t;
#  ifdef _WIN64
typedef __int64 utf8proc_ssize_t;
typedef unsigned __int64 utf8proc_size_t;
#  else
typedef int utf8proc_ssize_t;
typedef unsigned int utf8proc_size_t;
#  endif
#  ifndef __cplusplus
typedef unsigned char utf8proc_bool;
enum {false, true};
#  else
typedef bool utf8proc_bool;
#  endif
#else
#  include <stdbool.h>
#  include <inttypes.h>
typedef int8_t utf8proc_int8_t;
typedef uint8_t utf8proc_uint8_t;
typedef int16_t utf8proc_int16_t;
typedef uint16_t utf8proc_uint16_t;
typedef int32_t utf8proc_int32_t;
typedef uint32_t utf8proc_uint32_t;
typedef size_t utf8proc_size_t;
typedef ssize_t utf8proc_ssize_t;
typedef bool utf8proc_bool;
#endif
#include <limits.h>

#ifdef _WIN32
#  ifdef UTF8PROC_EXPORTS
#    define UTF8PROC_DLLEXPORT __declspec(dllexport)
#  else
#    define UTF8PROC_DLLEXPORT __declspec(dllimport)
#  endif
#elif __GNUC__ >= 4
#  define UTF8PROC_DLLEXPORT __attribute__ ((visibility("default")))
#else
#  define UTF8PROC_DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SSIZE_MAX
#define SSIZE_MAX ((size_t)SIZE_MAX/2)
#endif

#ifndef UINT16_MAX
#  define UINT16_MAX ~(utf8proc_uint16_t)0
#endif

/**
 * Option flags used by several functions in the library.
 */
typedef enum {
  /** The given UTF-8 input is NULL terminated. */
  UTF8PROC_NULLTERM  = (1<<0),
  /** Unicode Versioning Stability has to be respected. */
  UTF8PROC_STABLE    = (1<<1),
  /** Compatibility decomposition (i.e. formatting information is lost). */
  UTF8PROC_COMPAT    = (1<<2),
  /** Return a result with decomposed characters. */
  UTF8PROC_COMPOSE   = (1<<3),
  /** Return a result with decomposed characters. */
  UTF8PROC_DECOMPOSE = (1<<4),
  /** Strip "default ignorable characters" such as SOFT-HYPHEN or ZERO-WIDTH-SPACE. */
  UTF8PROC_IGNORE    = (1<<5),
  /** Return an error, if the input contains unassigned codepoints. */
  UTF8PROC_REJECTNA  = (1<<6),
  /**
   * Indicating that NLF-sequences (LF, CRLF, CR, NEL) are representing a
   * line break, and should be converted to the codepoint for line
   * separation (LS).
   */
  UTF8PROC_NLF2LS    = (1<<7),
  /**
   * Indicating that NLF-sequences are representing a paragraph break, and
   * should be converted to the codepoint for paragraph separation
   * (PS).
   */
  UTF8PROC_NLF2PS    = (1<<8),
  /** Indicating that the meaning of NLF-sequences is unknown. */
  UTF8PROC_NLF2LF    = (UTF8PROC_NLF2LS | UTF8PROC_NLF2PS),
  /** Strips and/or convers control characters.
   *
   * NLF-sequences are transformed into space, except if one of the
   * NLF2LS/PS/LF options is given. HorizontalTab (HT) and FormFeed (FF)
   * are treated as a NLF-sequence in this case.  All other control
   * characters are simply removed.
   */
  UTF8PROC_STRIPCC   = (1<<9),
  /**
   * Performs unicode case folding, to be able to do a case-insensitive
   * string comparison.
   */
  UTF8PROC_CASEFOLD  = (1<<10),
  /**
   * Inserts 0xFF bytes at the beginning of each sequence which is
   * representing a single grapheme cluster (see UAX#29).
   */
  UTF8PROC_CHARBOUND = (1<<11),
  /** Lumps certain characters together.
   *
   * E.g. HYPHEN U+2010 and MINUS U+2212 to ASCII "-". See lump.md for details.
   *
   * If NLF2LF is set, this includes a transformation of paragraph and
   * line separators to ASCII line-feed (LF).
   */
  UTF8PROC_LUMP      = (1<<12),
  /** Strips all character markings.
   *
   * This includes non-spacing, spacing and enclosing (i.e. accents).
   * @note This option works only with @ref UTF8PROC_COMPOSE or
   *       @ref UTF8PROC_DECOMPOSE
   */
  UTF8PROC_STRIPMARK = (1<<13),
} utf8proc_option_t;

/** @name Error codes
 * Error codes being returned by almost all functions.
 */
/** @{ */
/** Memory could not be allocated. */
#define UTF8PROC_ERROR_NOMEM -1
/** The given string is too long to be processed. */
#define UTF8PROC_ERROR_OVERFLOW -2
/** The given string is not a legal UTF-8 string. */
#define UTF8PROC_ERROR_INVALIDUTF8 -3
/** The @ref UTF8PROC_REJECTNA flag was set and an unassigned codepoint was found. */
#define UTF8PROC_ERROR_NOTASSIGNED -4
/** Invalid options have been used. */
#define UTF8PROC_ERROR_INVALIDOPTS -5
/** @} */

/* @name Types */

/** Holds the value of a property. */
typedef utf8proc_int16_t utf8proc_propval_t;

/** Struct containing information about a codepoint. */
typedef struct utf8proc_property_struct {
  /**
   * Unicode category.
   * @see utf8proc_category_t.
   */
  utf8proc_propval_t category;
  utf8proc_propval_t combining_class;
  /**
   * Bidirectional class.
   * @see utf8proc_bidi_class_t.
   */
  utf8proc_propval_t bidi_class;
  /**
   * @anchor Decomposition type.
   * @see utf8proc_decomp_type_t.
   */
  utf8proc_propval_t decomp_type;
  utf8proc_uint16_t decomp_mapping;
  utf8proc_uint16_t casefold_mapping;
  utf8proc_int32_t uppercase_mapping;
  utf8proc_int32_t lowercase_mapping;
  utf8proc_int32_t titlecase_mapping;
  utf8proc_int32_t comb1st_index;
  utf8proc_int32_t comb2nd_index;
  unsigned bidi_mirrored:1;
  unsigned comp_exclusion:1;
  /**
   * Can this codepoint be ignored?
   *
   * Used by @ref utf8proc_decompose_char when @ref UTF8PROC_IGNORE is
   * passed as an option.
   */
  unsigned ignorable:1;
  unsigned control_boundary:1;
  /**
   * Boundclass.
   * @see utf8proc_boundclass_t.
   */
  unsigned boundclass:4;
  /** The width of the codepoint. */
  unsigned charwidth:2;
} utf8proc_property_t;

/** Unicode categories. */
typedef enum {
  UTF8PROC_CATEGORY_CN  = 0, /**< Other, not assigned */
  UTF8PROC_CATEGORY_LU  = 1, /**< Letter, uppercase */
  UTF8PROC_CATEGORY_LL  = 2, /**< Letter, lowercase */
  UTF8PROC_CATEGORY_LT  = 3, /**< Letter, titlecase */
  UTF8PROC_CATEGORY_LM  = 4, /**< Letter, modifier */
  UTF8PROC_CATEGORY_LO  = 5, /**< Letter, other */
  UTF8PROC_CATEGORY_MN  = 6, /**< Mark, nonspacing */
  UTF8PROC_CATEGORY_MC  = 7, /**< Mark, spacing combining */
  UTF8PROC_CATEGORY_ME  = 8, /**< Mark, enclosing */
  UTF8PROC_CATEGORY_ND  = 9, /**< Number, decimal digit */
  UTF8PROC_CATEGORY_NL = 10, /**< Number, letter */
  UTF8PROC_CATEGORY_NO = 11, /**< Number, other */
  UTF8PROC_CATEGORY_PC = 12, /**< Punctuation, connector */
  UTF8PROC_CATEGORY_PD = 13, /**< Punctuation, dash */
  UTF8PROC_CATEGORY_PS = 14, /**< Punctuation, open */
  UTF8PROC_CATEGORY_PE = 15, /**< Punctuation, close */
  UTF8PROC_CATEGORY_PI = 16, /**< Punctuation, initial quote */
  UTF8PROC_CATEGORY_PF = 17, /**< Punctuation, final quote */
  UTF8PROC_CATEGORY_PO = 18, /**< Punctuation, other */
  UTF8PROC_CATEGORY_SM = 19, /**< Symbol, math */
  UTF8PROC_CATEGORY_SC = 20, /**< Symbol, currency */
  UTF8PROC_CATEGORY_SK = 21, /**< Symbol, modifier */
  UTF8PROC_CATEGORY_SO = 22, /**< Symbol, other */
  UTF8PROC_CATEGORY_ZS = 23, /**< Separator, space */
  UTF8PROC_CATEGORY_ZL = 24, /**< Separator, line */
  UTF8PROC_CATEGORY_ZP = 25, /**< Separator, paragraph */
  UTF8PROC_CATEGORY_CC = 26, /**< Other, control */
  UTF8PROC_CATEGORY_CF = 27, /**< Other, format */
  UTF8PROC_CATEGORY_CS = 28, /**< Other, surrogate */
  UTF8PROC_CATEGORY_CO = 29, /**< Other, private use */
} utf8proc_category_t;

/** Bidirectional character classes. */
typedef enum {
  UTF8PROC_BIDI_CLASS_L     = 1, /**< Left-to-Right */
  UTF8PROC_BIDI_CLASS_LRE   = 2, /**< Left-to-Right Embedding */
  UTF8PROC_BIDI_CLASS_LRO   = 3, /**< Left-to-Right Override */
  UTF8PROC_BIDI_CLASS_R     = 4, /**< Right-to-Left */
  UTF8PROC_BIDI_CLASS_AL    = 5, /**< Right-to-Left Arabic */
  UTF8PROC_BIDI_CLASS_RLE   = 6, /**< Right-to-Left Embedding */
  UTF8PROC_BIDI_CLASS_RLO   = 7, /**< Right-to-Left Override */
  UTF8PROC_BIDI_CLASS_PDF   = 8, /**< Pop Directional Format */
  UTF8PROC_BIDI_CLASS_EN    = 9, /**< European Number */
  UTF8PROC_BIDI_CLASS_ES   = 10, /**< European Separator */
  UTF8PROC_BIDI_CLASS_ET   = 11, /**< European Number Terminator */
  UTF8PROC_BIDI_CLASS_AN   = 12, /**< Arabic Number */
  UTF8PROC_BIDI_CLASS_CS   = 13, /**< Common Number Separator */
  UTF8PROC_BIDI_CLASS_NSM  = 14, /**< Nonspacing Mark */
  UTF8PROC_BIDI_CLASS_BN   = 15, /**< Boundary Neutral */
  UTF8PROC_BIDI_CLASS_B    = 16, /**< Paragraph Separator */
  UTF8PROC_BIDI_CLASS_S    = 17, /**< Segment Separator */
  UTF8PROC_BIDI_CLASS_WS   = 18, /**< Whitespace */
  UTF8PROC_BIDI_CLASS_ON   = 19, /**< Other Neutrals */
  UTF8PROC_BIDI_CLASS_LRI  = 20, /**< Left-to-Right Isolate */
  UTF8PROC_BIDI_CLASS_RLI  = 21, /**< Right-to-Left Isolate */
  UTF8PROC_BIDI_CLASS_FSI  = 22, /**< First Strong Isolate */
  UTF8PROC_BIDI_CLASS_PDI  = 23, /**< Pop Directional Isolate */
} utf8proc_bidi_class_t;

/** Decomposition type. */
typedef enum {
  UTF8PROC_DECOMP_TYPE_FONT      = 1, /**< Font */
  UTF8PROC_DECOMP_TYPE_NOBREAK   = 2, /**< Nobreak */
  UTF8PROC_DECOMP_TYPE_INITIAL   = 3, /**< Initial */
  UTF8PROC_DECOMP_TYPE_MEDIAL    = 4, /**< Medial */
  UTF8PROC_DECOMP_TYPE_FINAL     = 5, /**< Final */
  UTF8PROC_DECOMP_TYPE_ISOLATED  = 6, /**< Isolated */
  UTF8PROC_DECOMP_TYPE_CIRCLE    = 7, /**< Circle */
  UTF8PROC_DECOMP_TYPE_SUPER     = 8, /**< Super */
  UTF8PROC_DECOMP_TYPE_SUB       = 9, /**< Sub */
  UTF8PROC_DECOMP_TYPE_VERTICAL = 10, /**< Vertical */
  UTF8PROC_DECOMP_TYPE_WIDE     = 11, /**< Wide */
  UTF8PROC_DECOMP_TYPE_NARROW   = 12, /**< Narrow */
  UTF8PROC_DECOMP_TYPE_SMALL    = 13, /**< Small */
  UTF8PROC_DECOMP_TYPE_SQUARE   = 14, /**< Square */
  UTF8PROC_DECOMP_TYPE_FRACTION = 15, /**< Fraction */
  UTF8PROC_DECOMP_TYPE_COMPAT   = 16, /**< Compat */
} utf8proc_decomp_type_t;

/** Boundclass property. */
typedef enum {
  UTF8PROC_BOUNDCLASS_START              =  0, /**< Start */
  UTF8PROC_BOUNDCLASS_OTHER              =  1, /**< Other */
  UTF8PROC_BOUNDCLASS_CR                 =  2, /**< Cr */
  UTF8PROC_BOUNDCLASS_LF                 =  3, /**< Lf */
  UTF8PROC_BOUNDCLASS_CONTROL            =  4, /**< Control */
  UTF8PROC_BOUNDCLASS_EXTEND             =  5, /**< Extend */
  UTF8PROC_BOUNDCLASS_L                  =  6, /**< L */
  UTF8PROC_BOUNDCLASS_V                  =  7, /**< V */
  UTF8PROC_BOUNDCLASS_T                  =  8, /**< T */
  UTF8PROC_BOUNDCLASS_LV                 =  9, /**< Lv */
  UTF8PROC_BOUNDCLASS_LVT                = 10, /**< Lvt */
  UTF8PROC_BOUNDCLASS_REGIONAL_INDICATOR = 11, /**< Regional indicator */
  UTF8PROC_BOUNDCLASS_SPACINGMARK        = 12, /**< Spacingmark */
} utf8proc_boundclass_t;

/**
 * Array containing the byte lengths of a UTF-8 encoded codepoint based
 * on the first byte.
 */
UTF8PROC_DLLEXPORT extern const utf8proc_int8_t utf8proc_utf8class[256];

/**
 * Returns the utf8proc API version as a string MAJOR.MINOR.PATCH
 * (http://semver.org format), possibly with a "-dev" suffix for
 * development versions.
 */
UTF8PROC_DLLEXPORT const char *utf8proc_version(void);

/**
 * Returns an informative error string for the given utf8proc error code
 * (e.g. the error codes returned by @ref utf8proc_map).
 */
UTF8PROC_DLLEXPORT const char *utf8proc_errmsg(utf8proc_ssize_t errcode);

/**
 * Reads a single codepoint from the UTF-8 sequence being pointed to by `str`.
 * The maximum number of bytes read is `strlen`, unless `strlen` is
 * negative (in which case up to 4 bytes are read).
 *
 * If a valid codepoint could be read, it is stored in the variable
 * pointed to by `codepoint_ref`, otherwise that variable will be set to -1.
 * In case of success, the number of bytes read is returned; otherwise, a
 * negative error code is returned.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_iterate(const utf8proc_uint8_t *str, utf8proc_ssize_t strlen, utf8proc_int32_t *codepoint_ref);

/**
 * Check if a codepoint is valid (regardless of whether it has been
 * assigned a value by the current Unicode standard).
 *
 * @return 1 if the given `codepoint` is valid and otherwise return 0.
 */
UTF8PROC_DLLEXPORT utf8proc_bool utf8proc_codepoint_valid(utf8proc_int32_t codepoint);

/**
 * Encodes the codepoint as an UTF-8 string in the byte array pointed
 * to by `dst`. This array must be at least 4 bytes long.
 *
 * In case of success the number of bytes written is returned, and
 * otherwise 0 is returned.
 *
 * This function does not check whether `codepoint` is valid Unicode.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_encode_char(utf8proc_int32_t codepoint, utf8proc_uint8_t *dst);

/**
 * Look up the properties for a given codepoint.
 *
 * @param codepoint The Unicode codepoint.
 *
 * @returns
 * A pointer to a (constant) struct containing information about
 * the codepoint.
 * @par
 * If the codepoint is unassigned or invalid, a pointer to a special struct is
 * returned in which `category` is 0 (@ref UTF8PROC_CATEGORY_CN).
 */
UTF8PROC_DLLEXPORT const utf8proc_property_t *utf8proc_get_property(utf8proc_int32_t codepoint);

/** Decompose a codepoint into an array of codepoints.
 *
 * @param codepoint the codepoint.
 * @param dst the destination buffer.
 * @param bufsize the size of the destination buffer.
 * @param options one or more of the following flags:
 * - @ref UTF8PROC_REJECTNA  - return an error `codepoint` is unassigned
 * - @ref UTF8PROC_IGNORE    - strip "default ignorable" codepoints
 * - @ref UTF8PROC_CASEFOLD  - apply Unicode casefolding
 * - @ref UTF8PROC_COMPAT    - replace certain codepoints with their
 *                             compatibility decomposition
 * - @ref UTF8PROC_CHARBOUND - insert 0xFF bytes before each grapheme cluster
 * - @ref UTF8PROC_LUMP      - lump certain different codepoints together
 * - @ref UTF8PROC_STRIPMARK - remove all character marks
 * @param last_boundclass
 * Pointer to an integer variable containing
 * the previous codepoint's boundary class if the @ref UTF8PROC_CHARBOUND
 * option is used.  Otherwise, this parameter is ignored.
 *
 * @return
 * In case of success, the number of codepoints written is returned; in case
 * of an error, a negative error code is returned (@ref utf8proc_errmsg).
 * @par
 * If the number of written codepoints would be bigger than `bufsize`, the
 * required buffer size is returned, while the buffer will be overwritten with
 * undefined data.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_decompose_char(
  utf8proc_int32_t codepoint, utf8proc_int32_t *dst, utf8proc_ssize_t bufsize,
  utf8proc_option_t options, int *last_boundclass
);

/**
 * The same as @ref utf8proc_decompose_char, but acts on a whole UTF-8
 * string and orders the decomposed sequences correctly.
 *
 * If the @ref UTF8PROC_NULLTERM flag in `options` is set, processing
 * will be stopped, when a NULL byte is encounted, otherwise `strlen`
 * bytes are processed.  The result (in the form of 32-bit unicode
 * codepoints) is written into the buffer being pointed to by
 * `buffer` (which must contain at least `bufsize` entries).  In case of
 * success, the number of codepoints written is returned; in case of an
 * error, a negative error code is returned (@ref utf8proc_errmsg).
 *
 * If the number of written codepoints would be bigger than `bufsize`, the
 * required buffer size is returned, while the buffer will be overwritten with
 * undefined data.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_decompose(
  const utf8proc_uint8_t *str, utf8proc_ssize_t strlen,
  utf8proc_int32_t *buffer, utf8proc_ssize_t bufsize, utf8proc_option_t options
);

/**
 * Reencodes the sequence of `length` codepoints pointed to by `buffer`
 * UTF-8 data in-place (i.e., the result is also stored in `buffer`).
 *
 * @param buffer the (native-endian UTF-32) unicode codepoints to re-encode.
 * @param length the length (in codepoints) of the buffer.
 * @param options a bitwise or (`|`) of one or more of the following flags:
 * - @ref UTF8PROC_NLF2LS  - convert LF, CRLF, CR and NEL into LS
 * - @ref UTF8PROC_NLF2PS  - convert LF, CRLF, CR and NEL into PS
 * - @ref UTF8PROC_NLF2LF  - convert LF, CRLF, CR and NEL into LF
 * - @ref UTF8PROC_STRIPCC - strip or convert all non-affected control characters
 * - @ref UTF8PROC_COMPOSE - try to combine decomposed codepoints into composite
 *                           codepoints
 * - @ref UTF8PROC_STABLE  - prohibit combining characters that would violate
 *                           the unicode versioning stability
 *
 * @return
 * In case of success, the length (in bytes) of the resulting UTF-8 string is
 * returned; otherwise, a negative error code is returned (@ref utf8proc_errmsg).
 *
 * @warning The amount of free space pointed to by `buffer` must
 *          exceed the amount of the input data by one byte, and the
 *          entries of the array pointed to by `str` have to be in the
 *          range `0x0000` to `0x10FFFF`. Otherwise, the program might crash!
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_reencode(utf8proc_int32_t *buffer, utf8proc_ssize_t length, utf8proc_option_t options);

/**
 * Given a pair of consecutive codepoints, return whether a grapheme break is
 * permitted between them (as defined by the extended grapheme clusters in UAX#29).
 */
UTF8PROC_DLLEXPORT utf8proc_bool utf8proc_grapheme_break(utf8proc_int32_t codepoint1, utf8proc_int32_t codepoint2);


/**
 * Given a codepoint `c`, return the codepoint of the corresponding
 * lower-case character, if any; otherwise (if there is no lower-case
 * variant, or if `c` is not a valid codepoint) return `c`.
 */
UTF8PROC_DLLEXPORT utf8proc_int32_t utf8proc_tolower(utf8proc_int32_t c);

/**
 * Given a codepoint `c`, return the codepoint of the corresponding
 * upper-case character, if any; otherwise (if there is no upper-case
 * variant, or if `c` is not a valid codepoint) return `c`.
 */
UTF8PROC_DLLEXPORT utf8proc_int32_t utf8proc_toupper(utf8proc_int32_t c);

/**
 * Given a codepoint, return a character width analogous to `wcwidth(codepoint)`,
 * except that a width of 0 is returned for non-printable codepoints
 * instead of -1 as in `wcwidth`.
 *
 * @note
 * If you want to check for particular types of non-printable characters,
 * (analogous to `isprint` or `iscntrl`), use @ref utf8proc_category. */
UTF8PROC_DLLEXPORT int utf8proc_charwidth(utf8proc_int32_t codepoint);

/**
 * Return the Unicode category for the codepoint (one of the
 * @ref utf8proc_category_t constants.)
 */
UTF8PROC_DLLEXPORT utf8proc_category_t utf8proc_category(utf8proc_int32_t codepoint);

/**
 * Return the two-letter (nul-terminated) Unicode category string for
 * the codepoint (e.g. `"Lu"` or `"Co"`).
 */
UTF8PROC_DLLEXPORT const char *utf8proc_category_string(utf8proc_int32_t codepoint);

/**
 * Maps the given UTF-8 string pointed to by `str` to a new UTF-8
 * string, allocated dynamically by `malloc` and returned via `dstptr`.
 *
 * If the @ref UTF8PROC_NULLTERM flag in the `options` field is set,
 * the length is determined by a NULL terminator, otherwise the
 * parameter `strlen` is evaluated to determine the string length, but
 * in any case the result will be NULL terminated (though it might
 * contain NULL characters with the string if `str` contained NULL
 * characters). Other flags in the `options` field are passed to the
 * functions defined above, and regarded as described.
 *
 * In case of success the length of the new string is returned,
 * otherwise a negative error code is returned.
 *
 * @note The memory of the new UTF-8 string will have been allocated
 * with `malloc`, and should therefore be deallocated with `free`.
 */
UTF8PROC_DLLEXPORT utf8proc_ssize_t utf8proc_map(
  const utf8proc_uint8_t *str, utf8proc_ssize_t strlen, utf8proc_uint8_t **dstptr, utf8proc_option_t options
);

/** @name Unicode normalization
 *
 * Returns a pointer to newly allocated memory of a NFD, NFC, NFKD or NFKC
 * normalized version of the null-terminated string `str`.  These
 * are shortcuts to calling @ref utf8proc_map with @ref UTF8PROC_NULLTERM
 * combined with @ref UTF8PROC_STABLE and flags indicating the normalization.
 */
/** @{ */
/** NFD normalization (@ref UTF8PROC_DECOMPOSE). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFD(const utf8proc_uint8_t *str);
/** NFC normalization (@ref UTF8PROC_COMPOSE). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFC(const utf8proc_uint8_t *str);
/** NFD normalization (@ref UTF8PROC_DECOMPOSE and @ref UTF8PROC_COMPAT). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFKD(const utf8proc_uint8_t *str);
/** NFD normalization (@ref UTF8PROC_COMPOSE and @ref UTF8PROC_COMPAT). */
UTF8PROC_DLLEXPORT utf8proc_uint8_t *utf8proc_NFKC(const utf8proc_uint8_t *str);
/** @} */

#ifdef __cplusplus
}
#endif

#endif
#line 11 "imp.c"
#line 1 "api.h"
#ifndef SABIR_H
#define SABIR_H

/* This must be linked against the utf8proc library. Its source code is at:
 * https://github.com/JuliaLang/utf8proc
 */

#define SB_VERSION "0.1"

#include <stddef.h>

enum {
   SB_OK,      /* No error. */
   SB_EOPEN,   /* Cannot open model file. */
   SB_EMAGIC,  /* Not a model file. */
   SB_EMODEL,  /* Invalid model file. */
   SB_EIO,     /* I/O error. */
   SB_ENOMEM,  /* Out of memory. */
};

/* Returns a string describing an error code. */
const char *sb_strerror(int err);

struct sabir;

/* Loads a language detection model from a file.
 * Returns SB_OK on success, an error code otherwise.
 */
int sb_load(struct sabir **, const char *path);

/* Deallocates a model. */
void sb_dealloc(struct sabir *);

/* Returns the list of languages supported by a model.
 * The returned array is lexicographically sorted and NULL-terminated. If "nr"
 * is not NULL, fills it with the number of supported languages.
 */
const char *const *sb_langs(struct sabir *, size_t *nr);

/* Classifies a UTF-8 text chunk.
 * The returned pointer points to this object's internals. It should then not
 * be accessed after the model is deallocated.
 * This always returns a value, whether or not the text to classify is written
 * in one of the languages supported by this model.
 */
const char *sb_detect(struct sabir *, const void *text, size_t len);

/* Lower-level classification interface, useful when the input text is read from
 * a stream. The procedure is as follows:
 *   1. Call sb_init() to (re)initialize the classifier state.
 *   2. Call sb_feed() one or more times with pieces of the text to classify.
 *      It is assumed that these chunks are contiguous. They need not start or
 *      end on valid UTF-8 boundaries.
 *   3. Call sb_finish() once enough data has been gathered to obtain the best
 *      matching language.
 */
void sb_init(struct sabir *);
void sb_feed(struct sabir *, const void *chunk, size_t len);
const char *sb_finish(struct sabir *);

#endif
#line 12 "imp.c"

#ifdef SB_DEBUG
static const bool sb_debug = true;
#else
static const bool sb_debug = false;
#endif
bool sb_verbose;

#define SB_NGRAM_SIZE 4

struct sabir {
   size_t num_labels;
   size_t num_features;
   const char *const *labels;
   double *probs;
   uint8_t buf[SB_NGRAM_SIZE];
   size_t buf_pos;
   double model[];
};

const char *sb_strerror(int err)
{
   static const char *const tbl[] = {
      [SB_OK] = "no error",
      [SB_EOPEN] = "cannot open model",
      [SB_EMAGIC] = "not a model file",
      [SB_EMODEL] = "invalid model file",
      [SB_EIO] = "I/O error",
      [SB_ENOMEM] = "out of memory",
   };

   if (err >= 0 && (size_t)err < sizeof tbl / sizeof *tbl)
      return tbl[err];
   return "unknown error";
}

void sb_init(struct sabir *sb)
{
   sb->buf[0] = '\0';
   sb->buf_pos = 1;
   for (size_t i = 0; i < sb->num_labels; i++)
      sb->probs[i] = 0.;
}

static size_t sb_pad(size_t n, size_t align)
{
   return n + ((n + align - 1) & ~(align - 1));
}

/* Prevent overflow issues. */
#define SB_MAX_LABELS 600
#define SB_MAX_LABELS_LEN 2048
#define SB_MAX_FEATURES 300000

int sb_load(struct sabir **sbp, const char *path)
{
   *sbp = NULL;
   struct sabir *sb = NULL;

   FILE *fp = fopen(path, "r");
   if (!fp)
      return SB_EOPEN;

   /* Magic identifier and version. */
   if (fscanf(fp, "@ sabir 1\n") == EOF) {
      fclose(fp);
      return SB_EMAGIC;
   }

   /* Sections size. */
   size_t num_labels, labels_len, num_features;
   if (fscanf(fp, "> %zu %zu %zu\n", &num_labels, &labels_len, &num_features) != 3)
      goto bad_model;
   if (num_labels == 0 || num_labels > SB_MAX_LABELS)
      goto bad_model;
   if (labels_len == 0 || labels_len > SB_MAX_LABELS_LEN)
      goto bad_model;
   if (num_features == 0 || num_features > SB_MAX_FEATURES)
      goto bad_model;

   /* Compute memory offsets. */
   size_t ptrs_off = sb_pad(offsetof(struct sabir, model) + num_features * sizeof(*sb->model), alignof(char *));
   size_t strs_off = sb_pad(ptrs_off + (num_labels + 1) * sizeof(char **), alignof(char));
   size_t probs_off = sb_pad(strs_off + labels_len + num_labels + 1, alignof(double));
   size_t total = probs_off + num_labels * sizeof(double);

   /* Initialize our struct. */
   sb = malloc(total);
   if (!sb) {
      fclose(fp);
      return SB_ENOMEM;
   }
   sb->num_labels = num_labels;
   sb->num_features = num_features;
   sb->labels = (void *)((char *)sb + ptrs_off);
   sb->probs = (void *)((char *)sb + probs_off);
   sb_init(sb);

   char **ptrs = (void *)((char *)sb + ptrs_off);
   char *strs = (void *)((char *)sb + strs_off);

   /* Read labels. One per line. */
   size_t pos = 0;
   for (size_t i = 0; i < num_labels; i++) {
      char *line = fgets(&strs[pos], labels_len + num_labels + 1 - pos, fp);
      if (!line)
         goto bad_model;
      size_t len = strlen(line);
      if (len < 2 || line[len - 1] != '\n')
         goto bad_model;
      line[len - 1] = '\0';
      ptrs[i] = &strs[pos];
      pos += len;
   }
   ptrs[num_labels] = NULL;
   if (pos != labels_len + num_labels)
      goto bad_model;

   /* Read features. One per line. */
   for (size_t i = 0; i < num_features; i++) {
      uint64_t n;
      if (fscanf(fp, "%"SCNu64"\n", &n) != 1 || n > DBL_MAX - 1)
         goto bad_model;
      sb->model[i] = log(n + 1);
   }

   /* Should have reached the end of the file by now. */
   if (getc(fp) != EOF)
      goto bad_model;

   *sbp = sb;
   fclose(fp);
   return SB_OK;

bad_model: {
   int err = ferror(fp);
   fclose(fp);
   free(sb);
   return err ? SB_EIO : SB_EMODEL;
}
}

void sb_dealloc(struct sabir *sb)
{
   free(sb);
}

const char *const *sb_langs(struct sabir *sb, size_t *nr)
{
   if (nr)
      *nr = sb->num_labels;
   return sb->labels;
}

static uint32_t sb_hash_feature(const uint8_t s[static SB_NGRAM_SIZE], size_t pos)
{
   uint32_t h = 1315423911;
   for (size_t i = 0; i < SB_NGRAM_SIZE; i++)
      h ^= (h << 5) + s[(pos + i) % SB_NGRAM_SIZE] + (h >> 2);
   return h;
}

static uint32_t sb_hash_lang(uint32_t h, const char *lang)
{
   while (*lang)
      h ^= (h << 5) + (uint8_t)*lang++ + (h >> 2);
   return h;
}

static bool sb_is_letter(int32_t c)
{
   const utf8proc_property_t *p = utf8proc_get_property(c);

   switch (p->category) {
   case UTF8PROC_CATEGORY_LU:
   case UTF8PROC_CATEGORY_LL:
   case UTF8PROC_CATEGORY_LT:
   case UTF8PROC_CATEGORY_LM:
   case UTF8PROC_CATEGORY_LO:
      return true;
   default:
      return false;
   }
}

static void sb_report(const uint8_t *gram, size_t pos, const char *lang,
                      uint32_t hash, double prob)
{
   for (size_t i = 0; i < SB_NGRAM_SIZE; i++) {
      int c = gram[(pos + i) % SB_NGRAM_SIZE];
      printf("%02x", c);
   }
   printf(" %s %"PRIu32" %la\n", lang, hash, prob);
}

static void sb_update_probs(struct sabir *sb,
                            const uint8_t gram[static SB_NGRAM_SIZE],
                            size_t pos)
{
   uint32_t h1 = sb_hash_feature(gram, pos);

   for (size_t i = 0; i < sb->num_labels; i++) {
      uint32_t h2 = sb_hash_lang(h1, sb->labels[i]);
      double prob = sb->model[h2 % sb->num_features];
      sb->probs[i] += prob;
      if (sb_debug && sb_verbose)
         sb_report(gram, pos, sb->labels[i], h2, prob);
   }
}

static void sb_process(struct sabir *sb, const uint8_t *text, ssize_t len)
{
   uint8_t *buf = sb->buf;
   size_t pos = sb->buf_pos;

   ssize_t clen;
   for (ssize_t i = 0; i < len; i += clen) {
      int32_t c;
      clen = utf8proc_iterate(&text[i], len - i, &c);
      if (clen <= 0) {
         clen = 1;
         continue;
      }
      if (sb_is_letter(c)) {
         for (ssize_t j = 0; j < clen; j++) {
            buf[pos++ % SB_NGRAM_SIZE] = text[i + j];
            if (pos >= SB_NGRAM_SIZE)
               sb_update_probs(sb, buf, pos);
         }
      } else {
         buf[pos++ % SB_NGRAM_SIZE] = '\0';
         if (pos >= SB_NGRAM_SIZE)
            sb_update_probs(sb, buf, pos);
         buf[0] = '\0';
         pos = 1;
      }
   }   
   sb->buf_pos = pos;
}

#ifndef SSIZE_MAX
   #define SSIZE_MAX (SIZE_MAX / 2)
#endif

void sb_feed(struct sabir *sb, const void *chunk, size_t len)
{
   sb_process(sb, chunk, len < SSIZE_MAX ? len : SSIZE_MAX);
}

const char *sb_finish(struct sabir *sb)
{
   /* Handle the last ngram. */
   sb->buf[sb->buf_pos++ % SB_NGRAM_SIZE] = '\0';
   if (sb->buf_pos >= SB_NGRAM_SIZE)
      sb_update_probs(sb, sb->buf, sb->buf_pos);
   
   /* Just in case the caller attempts to call this function several times. */
   sb->buf_pos = 0;

   size_t best = 0;
   for (size_t i = 0; i < sb->num_labels; i++)
      if (sb->probs[i] > sb->probs[best])
         best = i;

   return sb->labels[best];
}

const char *sb_detect(struct sabir *sb, const void *text, size_t len)
{
   sb_init(sb);
   sb_process(sb, text, len < SSIZE_MAX ? len : SSIZE_MAX);
   return sb_finish(sb);
}
