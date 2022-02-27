/*
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
 * Runtime formatting of the buffer.
 * When at the root leve, end of scope is found, it means
 * no more items placed into buffer.
 *
 * @arg buf : uint8_t* - pointer to the buffer
 */
void tausch_format_buf( uint8_t *buf );

typedef struct
{
    /// Pointer to the buffer start, if sbuf == NULL, then the iter is invalid.
    uint8_t *sbuf;

    /// Pointer to the buffer end, if ebuf == NULL, then the iter is invalid.
    uint8_t *ebuf;

    /// Pointer to start of current iterator.
    uint8_t *idx;

    /// Pointer to index position of next item.
    uint8_t *next;

    /// Pointer to the value field or NULL,
    /** if val == next, then the iter is incomplete,
     * if val == next, then the iter is incomplete,
     * if val == next, then the iter is incomplete,
     * if val == NULL, then the value is null.
     */
    uint8_t *val;

    /// Tag value of the item.
    size_t tag;

    /// Length of the value part.
    size_t vlen;

    /// The scope depth of the structure.
    uint16_t scope;

    /// The l and c bits of the tag,
    /**  if lc == 4 then
     * the iterator points to end of buffer.
     */
    uint8_t lc;

} tausch_iter_t;

/**
 * Run-time initiation of the iterator
 *
 * @arg iter : tausch_iter_t* - pointer to the iterator to initialize
 * @arg buf : uint8_t* - pointer to the buffer
 * @arg size : size_t - amount of data in the buffer
 * @return tausch_iter_t* - pointer to the iter
 */
tausch_iter_t* tausch_iter_init( tausch_iter_t *iter, uint8_t *buf, size_t size )
;

/**
 * Compile-time initiation of the iterator
 *
 * @arg buf : uint8_t* - pointer to the buffer
 * @arg size : size_t - amount of data in the buffer
 */
#define TAUSCH_ITER_INIT( buf, size ) \
{\
	.ebuf = (uint8_t*)(buf) + (size), \
	.sbuf = (uint8_t*)(buf),\
	.idx = (uint8_t*)(buf), \
	.next = (uint8_t*)(buf), \
	.val = (uint8_t*)(buf), \
	.tag = ~(size_t)0, \
	.vlen = 0, \
	.scope = 0, \
	.lc = 0 \
}

/**
 * Reset the iterator to the beginning, keep ref to buffer.
 *
 * @param iter : tausch_iter_t* - the iterator to be reset
 * @return tausch_iter_t*
 */
tausch_iter_t* tausch_iter_reset( tausch_iter_t *iter )
;

/**
 * Blob structure that holds the size of memroy available and how many bytes is used in.
 *
 * The Blob is assumed by tauschema methods to be fully filled. If your blob is partially
 * filled and you need to keep track of all available memory, then create another temporary
 * Blob that does have exactly the amount of data in len field.
 *
 * @example
 *
 * TAUSCH_BLOB_NEW( mymemory, 100 ); // avoid allocating stack with huge blobs !
 *
 * void shortblob_example( void )
 * {
 *   // produce clone of the mymemory blob
 *   tausch_blob_t actual = mymemory;
 *   // now fill in the data, will also fill into mymemory.buf since it is the same
 *   actual.len = sltrlen( strncpy( actual.buf, "This is something", actual.len ) );
 *   // ready to use the blob
 *   use_the_blob( &actual );
 * }
 *
 * void slice_of_blob( tausch_blob_t *blob, size_t offset, size_t len )
 * {
 *   // produce clone of the blob
 *   tausch_blob_t actual = *blob;
 *   // now take slice out of the blob
 *   if( ((offset + len) > offset) && ((offset+len) <= actual.len) )
 *   {
 *      // no memory access fault would happen
 *      actual.buf += offset;
 *      actual.len = len;
 *      use_the_blob( &actual );
 *   }
 * }
 *
 */
typedef struct
{
    /// Pointer to the buffer start
    uint8_t *buf;
    /// length of the buffer in bytes
    size_t len;
} tausch_blob_t;

/**
 * Compile time creation of blob. It does reserve memory in stack or globals.
 *
 * @arg name - the name of the blob variable
 * @arg size - numbr of bytes the blob shall hold
 */
#define TAUSCH_BLOB_NEW( name, size )\
    uint8_t name ## _buf[ size ];\
    tausch_blob_t name = { .buf = name ## _buf, .len = size, .count = 0 }

/**
 * Produce result blob that references only slice of orig blob.
 *
 * @param result : tausch_blob_t* - the memory field of produced blob reference
 * @param orig : tausch_blob_t* - the memory fileld of original blob reference
 * @param offset : size_t - the offset in the original blob
 * @param len : size_t - the amount of bytes in resulting blob reference
 * @return
 */
tausch_blob_t* tausch_blob_slice( tausch_blob_t *result, tausch_blob_t *orig, size_t offset, size_t len );

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
#define tausch_iter_read( iter, value ) _Generic((value), \
	bool*:          tausch_iter_read_bool( (iter), (bool*)(value) ), \
    char*:          restricted__use_blob_instead(), \
    tausch_blob_t*: tausch_iter_read_blob( (iter), (tausch_blob_t*)(value) ), \
    void*:          tausch_iter_vlen( (iter) ), \
    default:        tausch_iter_read_typX( (iter), (uint8_t*)(value), sizeof((value)[0]) ) \
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
#define tausch_iter_write( iter, tag, value ) _Generic((value), \
	bool*:          tausch_iter_write_bool( (iter), (size_t)(tag), (bool*)(value) ), \
    char*:          tausch_iter_write_utf8( (iter), (size_t)(tag), (char*)(value) ), \
	tausch_blob_t*: tausch_iter_write_blob( (iter), (size_t)(tag), (tausch_blob_t*)(value) ), \
	void*:          tausch_iter_write_typX( (iter), (size_t)(tag), (uint8_t*)NULL, ((iter)->vlen) ), \
	default:        tausch_iter_write_typX( (iter), (size_t)(tag), (uint8_t*)(value), sizeof((value)[0]) ) \
)

/**
 * Return true if the character is EOF (End of File)
 *
 * @arg iter : tausch_iter_t* - pointer to the iterator
 * @return true if it is byte of EOF
 */
bool tausch_iter_is_eof( tausch_iter_t *iter )
;

/**
 * Return true if the characer is EOS (End of Scope) or EOF (End of File)
 *
 * @arg iter : tausch_iter_t* - pointer to the iterator
 * @return true if it is byte of EOS or EOF
 */
bool tausch_iter_is_end( tausch_iter_t *iter )
;

/**
 * Format the end of the buffer
 *
 * @param iter : tausch_iter_t - the iterator
 * @return tausch_iter_t
 */
tausch_iter_t* tausch_iter_format( tausch_iter_t *iter )
;

/**
 * Return true if iterator is ok false if it is broken
 *
 * @arg iter : tausch_iter_t* - the iterator to verify
 */
bool tausch_iter_is_ok( tausch_iter_t *iter )
;

/**
 * Check if the iterator is complete
 *
 * @return true if all the elements of TLV have been parsed
 */
bool tausch_iter_is_complete( tausch_iter_t *iter )
;

/**
 * @return true if the value is null
 */
bool tausch_iter_is_null( tausch_iter_t *iter )
;

/**
 * Check if the iterator has not parsed TLV
 *
 * @return true if the iterator is clean for write
 */
bool tausch_iter_is_clean( tausch_iter_t *iter )
;

/**
 * @return size of the stuffing when the iter is stuffing, otherwise 0
 */
size_t tausch_iter_is_stuffing( tausch_iter_t *iter )
;

/**
 * Return if the iterator is at start of scope
 *
 * @param iter : tausch_iter_t*
 * @return bool
 */
bool tausch_iter_is_scope( tausch_iter_t *iter )
;

/**
 * Return amount of free space in buffer
 *
 * @param iter : tausch_iter_t*
 * @return size_t
 */
size_t tausch_iter_buff_free( tausch_iter_t *iter )
;

/**
 * Decode from binary buffer the variable length unsigned integer
 *
 * @arg iter - the iterator from where to read
 *
 * @return the decoded tag value, or ~0 (all bits set) on failure
 *
 */
size_t tausch_iter_decode_vluint( tausch_iter_t *iter )
;

/**
 * Encode to binary buffer the variable length unsigned integer
 *
 * @arg iter - the iterator where towrite
 * @arg val - the value to write
 *
 * @return true on success
 * @return false on failure
 */
bool tausch_iter_encode_vluint( tausch_iter_t *iter, size_t val )
;

/**
 * Calculate memory length of the resulting vluint
 *
 * @arg val - the value to encode
 *
 * @return number of bytes it would take
 */
size_t tausch_vluint_len( size_t val )
;

/**
 * Decodes from binary buffer next TLV item and stores info inside iter element.
 * It does not skip over stuffing. It stays onto the current scope and jumps
 * over the subscope. Also it stays into end of scope or end of file tag.
 *
 * When returning false, then there was end of scope or end of file or the
 * next element does not exist or the iterator becomes invalid.
 *
 * @return true on success, false on failure.
 */
bool tausch_iter_next( tausch_iter_t *iter )
;

/**
 * Enter the iterator into subscope if the iterator is at the beginning of scope.
 * Iterator stays to position and becomes clean.
 *
 * @param iter : tausch_iter_t* - the iterator
 * @return bool - true if succeeded false if failed
 */
bool tausch_iter_enter_scope( tausch_iter_t *iter )
;

/**
 * Exit the iterator from current scope.
 * It does advance iterator to the next element but keeps it clean.
 *
 *
 * @param iter : tausch_iter_t* - the iterator
 * @return bool - true on success, flase on serious failure
 */
bool tausch_iter_exit_scope( tausch_iter_t *iter )
;

/**
 * Advance the iterator to the next stuffing in the scope or next element
 * after end of scope or to the EOF
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_iter_go_to_stuffing( tausch_iter_t *iter )
;

/**
 * Advance the iterator to the next tag in the scope.
 *
 * @arg tag - the tag that is looked for
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed
 */
bool tausch_iter_go_to_tag( tausch_iter_t *iter, size_t tag )
;

/**
 * Calculate the amount of memory needed for TLV of primitive in message. In case of error
 * it does return 0.
 *
 * When vlen is 0 then tag only length is returned.
 *
 * @param tag : size_t - the tag id
 * @param vlen : size_t - the length needed for value
 * @return size_t - number of bytes needed
 */
size_t tausch_tlv_size( size_t tag, size_t vlen )
;

/**
 * Calculate the amount of value field length avialablte when for TLV the total space is memlen
 *
 * @param tag : size_t - the tag id
 * @param memlen : size_t - the memory space available
 * @return size_t amount of value length available, memlen if not possible at all
 */
size_t tausch_tlv_vlen( size_t tag, size_t memlen )
;

/**
 * Turn the element pointed by iterator into stuffing.
 * Arg len does apply only if the iterator points at EOF otherwise the area between idx and next is
 * turned into stuffing.
 *
 * @arg len - the amount of bytes to write.
 *
 * @return true if the stuffing was successful.
 * @return false if the stuffing was unsuccessful.
 */
bool tausch_iter_write_stuffing( tausch_iter_t *iter, size_t len )
;

/**
 * Erase the current item, turn fully into stuffing.
 *
 * Iterator may point to primitive.
 * Iterator may point to scope opening, then the entire scope is turned into stuffung.
 *
 * @param iter
 * @return
 */
bool tausch_iter_erase( tausch_iter_t *iter )
;

/**
 * Open new scope on the binary stream. Scopes are COLLECTION or VARIADIC
 *
 * @arg iter - the iterator where to append the scope
 * @arg tag - the tag of the scope
 *
 * @return false on failure
 * @return true on success
 */
bool tausch_iter_write_scope( tausch_iter_t *iter, size_t tag )
;

/**
 * Close lastly open scope
 *
 * @arg iter - the iterator where to append the scope
 *
 * @return false on failure
 * @return true on success
 */
bool tausch_iter_write_end( tausch_iter_t *iter )
;

/**
 * Read the iterator value field as BOOL
 *
 * @return true if the read was successful
 * @return false if the read was unsuccessful
 */
bool tausch_iter_read_bool( tausch_iter_t *iter, bool *value )
;

/**
 * Write the iterator value field as BOOL.
 * When the value is NULL then the value is considered true and it is stored
 * in the field in shortest possible way (tag only). Otherwise as uint8_t
 * or the entire existing field is filled with value.
 *
 * @arg iter - the iterator
 * @arg value - pointer to value field, if NULL then it is considered true
 *
 * @return true if the write was successful
 * @return false if the write was unsuccessful
 */
bool tausch_iter_write_bool( tausch_iter_t *iter, size_t tag, bool *value )
;

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
size_t tausch_iter_read_typX( tausch_iter_t *iter, uint8_t *value, size_t len )
;

/**
 * Read the iterator value field as any blob, the blob must have more memory than
 * the iterator value field. If less than blob len is read, the remainder of blob is
 * filled with 0 bytes.
 *
 *
 * @arg iter - the iterator
 * @arg value - pointer to the blob where to copy the value over
 *
 * @return 0 on failure
 * @return number of bytes read out
 */
size_t tausch_iter_read_blob( tausch_iter_t *iter, tausch_blob_t *value )
;

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
size_t tausch_iter_write_typX( tausch_iter_t *iter, size_t tag, uint8_t *value, size_t len )
;

/**
 * Write the blob into the value field.
 * Less data can be written from blob buffer, loog for the iter->vlen, how much was taken
 *
 * @arg iter - the iterator
 * @arg tag - the tag value
 * @arg value - the pointer to blob
 *
 * @return 0 on failure
 * @return size written on success
 */
size_t tausch_iter_write_blob( tausch_iter_t *iter, size_t tag, tausch_blob_t *value )
;

/**
 * Write the string into the value field. String is handled as blob with 0 ending buffer.
 *
 *
 * @arg iter - the iterator
 * @arg tag - the tag value
 * @arg value - the pointer to string
 *
 * @return 0 on failure
 * @return size written on success
 */
size_t tausch_iter_write_utf8( tausch_iter_t *iter, size_t tag, char *value )
;

/**
 * Get the length of the TLV value field
 */
size_t tausch_iter_vlen( tausch_iter_t *iter );

/**
 * Function that shall never be implemented => linking must fail
 */
void restricted__use_blob_instead( void );

#endif //__TAUSCHEMA_CODEC_C__
