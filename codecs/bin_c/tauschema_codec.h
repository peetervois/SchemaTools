/**
 * TauSchema Codec C
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of author nor the names of
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __TAUSCHEMA_CODEC_C__
#define __TAUSCHEMA_CODEC_C__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Enumeration of the TLV primitives
 */
typedef enum
{
	TSCH_BOOL = 0,
	TSCH_UINT,
	TSCH_UINT_8,
	TSCH_UINT_16,
	TSCH_UINT_32,
	TSCH_UINT_64,
	TSCH_UINT_128,
	TSCH_SINT,
	TSCH_SINT_8,
	TSCH_SINT_16,
	TSCH_SINT_32,
	TSCH_SINT_64,
	TSCH_SINT_128,
	TSCH_FLOAT,
	TSCH_FLOAT_32,
	TSCH_FLOAT_64,
	TSCH_FLOAT_128,
	TSCH_UTF8,
	TSCH_BLOB,
	TSCH_COLLECTION,
	TSCH_VARIADIC
}tausch_ntype_t;

/**
 * Structure of defining the flat-tree row
 */
typedef struct
{
	uint16_t	item; // Binary coded item number
	char		*name; // Name of the item (for human / json)
	char		*desc; // Description of the item, help text
	uint16_t	ntype; // type number
	uint16_t	sub; // table index to first subitem
	uint16_t	next; // table index to next item on same scope
}tausch_flatrow_t;


typedef struct
{
	uint8_t		*ebuf; 	// Pointer to the buffer end
						// if ebuf == NULL, then the iter is invalid
	uint8_t 	*idx; 	// pointer to start of current iterator
	uint8_t		*next; 	// pointer to index position of next item
	uint8_t		*val; 	// Pointer to the value field or NULL
						// if val == next, then the iter is incomplete
						// if val == NULL, then the value is null
	size_t		tag; 	// tag value of the item
	size_t		vlen; 	// length of the value part
	uint16_t    scope; 	// the scope depth of the structure
	uint8_t		lc;  	// the l and c bits of the tag
}tausch_iter_t;

#define TAUSCH_ITER_INIT( buf, size ) \
{\
	.ebuf = (uint8_t*)(buf) + (size), \
	.idx = (uint8_t*)(buf), \
	.next = (uint8_t*)(buf), \
	.val = (uint8_t*)(buf), \
	.tag = ~(size_t)0, \
	.vlen = 0, \
	.scope = 0, \
	.lc = 0 \
}

typedef struct
{
	uint8_t		*buf; // Pointer to the buffer start
	size_t		len; // length of the buffer in bytes
}tausch_blob_t;

/**
 * Methods for copying data out from the TLV value field.
 * If value is NULL no copy happens, instead length of value is returned.
 *
 * @arg iter - pointer to iterator object
 * @arg value - pointer to variable field where to copy
 *
 * @return 0 unsuccessful
 * @return number of bytes read out
 */
#define tausch_read( iter, value ) _Generic((value), \
    bool*:          tausch_read_bool( (iter), (value) ), \
    char*:          restricted__use_blob_instead(), \
    tausch_blob_t*: tausch_read_blob( (iter), (value) ), \
    void*:          tausch_iter_vlen( (iter) ), \
    default:        tausch_read_typX( (iter), (uint8_t*)(value), sizeof((value)[0]) ), \
)

/**
 * Methods for copying data from memory to TLV binary
 *
 * @arg iter - pointer to iterator object
 * @arg tag - value of the tag
 * @arg var - pointer to variable field
 *
 * @return 0 on failure
 * @return number of value bytes written
 */
#define tausch_write( iter, tag, value ) _Generic((value), \
    bool*:          tausch_write_bool( (iter), (size_t)(tag), (bool*)(value) ), \
    char*:          tausch_write_utf8( (iter), (size_t)(tag), (char*)(value) ), \
	tausch_blob_t*: tausch_write_blob( (iter), (size_t)(tag), (tausch_blob_t*)(value) ), \
	void*:          tausch_write_typX( (iter), (size_t)(tag), (uint8_t*)NULL, 0 ), \
	default:        tausch_write_typX( (iter), (size_t)(tag), (uint8_t*)(value), sizeof((value)[0]) ) \
)

/**
 * Return true if iterator is ok false if it is broken
 */
bool tausch_iter_is_ok( tausch_iter_t *iter );

/**
 * Return true if the iterator is complete
 */
bool tausch_iter_is_complete( tausch_iter_t *iter );

/**
 * Return true if the value is null
 */
bool tausch_iter_is_null( tausch_iter_t *iter );

/**
 * Return true if the iterator is clean for write
 */
bool tausch_iter_is_clean( tausch_iter_t *iter );

/**
 * Return size of the stuffing when the iter is stuffing, otherwise 0
 */
size_t tausch_iter_is_stuffing( tausch_iter_t *iter );

/**
 * Decode from binary buffer the variable length unsigned integer
 *
 * @arg iter - the iterator from where to read
 *
 * @return the decoded tag value, or ~0 (all bits set) on failure
 *
 */
size_t tausch_decode_vluint( tausch_iter_t *iter );

/**
 * Encode to binary buffer the variable length unsigned integer
 *
 * @arg iter - the iterator where towrite
 * @arg val - the value to write
 *
 * @return true on success
 * @return false on failure
 */
bool tausch_encode_vluint( tausch_iter_t *iter, size_t val );

/**
 * Calculate memory length of the resulting vluint
 *
 * @arg val - the value to encode
 *
 * @return number of bytes it would take
 */
size_t tausch_vluint_len( size_t val );

/**
 * Decodes from binary buffer next TLV item and stores info
 * inside iter element. It does not skip over stuffing.
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_next( tausch_iter_t *iter );

/**
 * Call this method when writing of the element has been finished.
 * It does advance the iterator so that new element may be written
 * right after the current element. Note that in case you are over-
 * writing the existing binary TLV, then use decode_next() instead.
 * This method is used when you are appending to the tlv, it does
 * destroy the binary structure beneath.
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_write_next( tausch_iter_t *iter );

/**
 * Advance the iterator to the element next item on the same scope
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_to_next( tausch_iter_t *iter );

/**
 * Advance the iterator to the element next item after end of current scope
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_to_eoscope( tausch_iter_t *iter );

/**
 * Advance the iterator to the next stuffing in the scope or next element
 * after end of scope
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_to_stuffing( tausch_iter_t *iter );

/**
 * Turn the element pointed by iterator into stuffing.
 * If the iterator is incomplete, then it creates.
 *
 * @arg len - the amount of bytes to write
 *
 * @return true if the stuffing was successful
 * @return false if the stuffing was unsuccessful
 */
bool tausch_write_stuffing( tausch_iter_t *iter, size_t len );

/**
 * Open new scope on the binary stream. Scopes are COLLECTION or VARIADIC
 *
 * @arg iter - the iterator where to append the scope
 * @arg tag - the tag of the scope
 *
 * @return false on failure
 * @return true on success
 */
bool tausch_write_scope( tausch_iter_t *iter, size_t *tag );

/**
 * Close lastly open scope
 *
 * @arg iter - the iterator where to append the scope
 *
 * @return false on failure
 * @return true on success
 */
bool tausch_write_end( tausch_iter_t *iter );

/**
 * Read the iterator value field as BOOL
 *
 * @return true if the read was successful
 * @return false if the read was unsuccessful
 */
bool tausch_read_bool( tausch_iter_t *iter, bool *value );

/**
 * Write the iterator value field as BOOL.
 * When the value is NULL then the value is considered true and it is stored
 * in the field in shortest possible way (tag only). Otherwise as uint8_t
 * or the entire existing field is filled with value.
 *
 * @arg iter - the iterator
 * @arg value - pointer to value field, if NULL then it is considered true
 *
 * @return true if the read was successful
 * @return false if the read was unsuccessful
 */
bool tausch_write_bool( tausch_iter_t *iter, size_t tag, bool *value );

/**
 * Read the iterator value field as any finite value
 *
 * @arg iter - the iterator
 * @arg value - pointer to the value to copy the value over
 * @arg len - length of the finite value memory field
 *
 * @return 0 on failure
 * @return number of bytes read out
 */
size_t tausch_read_typX( tausch_iter_t *iter, uint8_t *value, size_t len );

/**
 * Write to the location of iterator any finite value. When the iter already
 * contains an element, it does verify it the tags match and the lengths match.
 * When value is NULL, and length is 0 a null item will be written.
 * When value is NULL and length is >0 all value field is replaced with 0x00
 *
 * @arg iter - the iterator
 * @arg tag - the tag value
 * @arg value - pointer to the value to copy the value from
 * @arg len - length of the finite value memory field
 *
 * @return 0 on failure
 * @return number of value bytes written, for null return 1
*/
size_t tausch_write_typX( tausch_iter_t *iter, size_t tag, uint8_t *value, size_t len );

/**
 * Read the iterator value field as UINT-X
 *
 * @return 0 unsuccessful
 * @return number of bytes read out
 */
size_t tausch_read_blob( tausch_iter_t *iter, tausch_blob_t *value );

/**
 * Write the blob into the value field.
 * When overwriting the blob size must match with the blob size in binary
 *
 * @arg iter - the iterator
 * @arg tag - the tag value
 * @arg value - the pointer to blob
 *
 * @return 0 on failure
 * @return size written on success
 */
size_t tausch_write_blob( tausch_iter_t *iter, size_t tag, tausch_blob_t *value );

/**
 * Write the string into the value field.
 * When overwriting the string size must be smaller equal to the binary size.
 * When string is shorter, it is padded with zeroes.
 *
 * @arg iter - the iterator
 * @arg tag - the tag value
 * @arg value - the pointer to string
 *
 * @return 0 on failure
 * @return size written on success
 */
size_t tausch_write_utf8( tausch_iter_t *iter, size_t tag, char *value );

/**
 * Get the length of the TLV value field
 */
size_t tausch_iter_vlen( tausch_iter_t *iter );


#endif //__TAUSCHEMA_CODEC_C__
