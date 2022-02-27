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

#include "tauschema_codec.h"
#include "string.h"

/**
 * Runtime formatting of the buffer.
 * When at the root leve, end of scope is found, it means
 * no more items placed into buffer.
 *
 * @arg buf : uint8_t* - pointer to the buffer
 */
void tausch_format_buf( uint8_t *buf )
{
    buf[0] = 0x07;   // EOF marker END with tag > 0
}

/**
 * Run-time initiation of the iterator
 *
 * @arg iter : tausch_iter_t* - pointer to the iterator to initialize
 * @arg buf : uint8_t* - pointer to the buffer
 * @arg size : size_t - amount of data in the buffer
 * @return tausch_iter_t* - pointer to the iter
 */
tausch_iter_t* tausch_iter_init( tausch_iter_t *iter, uint8_t *buf, size_t size )
{
    tausch_iter_t tm = TAUSCH_ITER_INIT( buf, size );
    *iter = tm;
    return iter;
}

/**
 * Reset the iterator to the beginning, keep ref to buffer.
 *
 * @param iter : tausch_iter_t* - the iterator to be reset
 * @return tausch_iter_t*
 */
tausch_iter_t* tausch_iter_reset( tausch_iter_t *iter )
{
    iter->idx = iter->sbuf;
    iter->next = iter->sbuf;
    iter->val = iter->sbuf;
    iter->tag = ~(size_t)0;
    iter->vlen = 0;
    iter->scope = 0;
    iter->lc = 0;
    return iter;
}

/**
 * Return true if the character is EOF (End of File)
 *
 * @arg iter : tausch_iter_t* - pointer to the iterator
 * @return true if it is byte of EOF
 */
bool tausch_iter_is_eof( tausch_iter_t *iter )
{
    return ( (iter->lc == 3) && (iter->tag > 0) && (iter->tag != ~0));
}

/**
 * Return true if the characer is EOS (End of Scope) or EOF (End of File)
 *
 * @arg iter : tausch_iter_t* - pointer to the iterator
 * @return true if it is byte of EOS or EOF
 */
bool tausch_iter_is_end( tausch_iter_t *iter )
{
    return (iter->lc == 3);
}

/**
 * Format the end of the buffer
 *
 * @param iter : tausch_iter_t - the iterator
 * @return tausch_iter_t
 */
tausch_iter_t* tausch_iter_format( tausch_iter_t *iter )
{
    *iter->next = 7;
    return iter;
}

/**
 * Return true if iterator is ok false if it is broken
 *
 * @arg iter : tausch_iter_t* - the iterator to verify
 */
bool tausch_iter_is_ok( tausch_iter_t *iter )
{
    bool notok = false;
    notok |= iter->ebuf == NULL;   // there is no buffer pointed
    notok |= iter->idx == NULL;   // idx is pointing to nothing
    notok |= iter->idx >= iter->ebuf;   // index is out of buffer or end of buffer
    notok |= iter->next == NULL;   // next is pointing to nothing
    notok |= iter->next > iter->ebuf;   // next is pointing out of buffer
    notok |= (iter->next == iter->idx) && (iter->val != iter->next);   // iter->val has incorrect value
    notok |= iter->sbuf == NULL;   // there is no buffer pointed
    return !notok;
}

/**
 * Check if the iterator is complete
 *
 * @return true if all the elements of TLV have been parsed
 */
bool tausch_iter_is_complete( tausch_iter_t *iter )
{
    // we do not check if iter is ok here, because that would reduce dimensions
    return iter->val != iter->next;
}

/**
 * @return true if the value is null
 */
bool tausch_iter_is_null( tausch_iter_t *iter )
{
    return iter->val == NULL;
}

/**
 * Check if the iterator has not parsed TLV
 *
 * @return true if the iterator is clean for write
 */
bool tausch_iter_is_clean( tausch_iter_t *iter )
{
    if( !tausch_iter_is_ok( iter ) ) return false;
    return (iter->next == iter->idx) && (iter->val == iter->next);
}

/**
 * @return size of the stuffing when the iter is stuffing, otherwise 0
 */
size_t tausch_iter_is_stuffing( tausch_iter_t *iter )
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( iter->tag != 0 ) return 0;
    return iter->next - iter->idx;
}

/**
 * Return if the iterator is at start of scope
 *
 * @param iter : tausch_iter_t*
 * @return bool
 */
bool tausch_iter_is_scope( tausch_iter_t *iter )
{
    return ( (iter->lc & 3) == 1);
}

/**
 * Return amount of free space in buffer
 *
 * @param iter : tausch_iter_t*
 * @return size_t
 */
size_t tausch_iter_buff_free( tausch_iter_t *iter )
{
    if( iter->ebuf == NULL ) return 0;
    tausch_iter_t ti = *iter;
    while( (!tausch_iter_is_eof( &ti )) && tausch_iter_is_ok( &ti ) )
    {
        (void)tausch_iter_exit_scope( &ti );
    }
    if( tausch_iter_is_ok( &ti ) )
    {
        return ti.ebuf - ti.idx - 1;
    }
    return 0;
}

/**
 * Decode from binary buffer the variable length unsigned integer
 *
 * @arg iter - the iterator from where to read
 *
 * @return the decoded tag value, or ~0 (all bits set) on failure
 *
 */
size_t tausch_iter_decode_vluint( tausch_iter_t *iter )
{
    size_t rv = 0;
    size_t x = 0;
    uint32_t s = 0;
    if( iter->next != iter->val ) return ~0;
    do
    {
        if( !tausch_iter_is_ok( iter ) ) return ~0;
        if( iter->next >= iter->ebuf )
        {
            iter->ebuf = NULL;
            return ~0;
        }
        x = (size_t) (*iter->next);
        if( (s + 1) < (sizeof(size_t) * 8) )
        {
            // we do not support bigger numbers for vluint
            rv |= (x & 0x7f) << s;
            s += 7;
        }
        iter->next += 1;
    }
    while( ( (x & 0x80) == 0x80) && (s < (sizeof(size_t) * 8)) );
    iter->val = iter->next;
    return rv;
}

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
{
    size_t x = val;
    uint8_t b = 0;
    if( tausch_iter_is_complete( iter ) ) return false;   // the iterator is complete already
    do
    {
        if( !tausch_iter_is_ok( iter ) ) return false;
        b = (x & (~0x7f)) ? 0x80 : 0x00;
        b |= x & 0x7f;
        * (iter->next++) = b;
        x >>= 7;
    }
    while( x );
    iter->val = iter->next;
    return true;
}

/**
 * Calculate memory length of the resulting vluint
 *
 * @arg val - the value to encode
 *
 * @return number of bytes it would take
 */
size_t tausch_vluint_len( size_t val )
{
    unsigned int *x = (unsigned int*)&val;
    size_t rv = 0;   // now: rv is number of msbits zeroed
    for( int i = (sizeof(size_t) / sizeof(unsigned int) - 1); i >= 0; i-- )
    {
        if( x[i] == 0 )
        {
            rv += sizeof(unsigned int) << 3;
        }
        else
        {
            rv += __builtin_clz( x[i] );
            break;
        }
    }
    rv = (sizeof(size_t) << 3) - rv;   // now: rv is number of significant bits
    rv = (rv + 6) / 7;   // now: rv is number of bytes
    return rv > 0 ? rv : 1;
}

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
{
    size_t tag = 0;
    size_t len = 0;
    uint16_t sco = iter->scope;
    bool rv = true;

    do
    {
        //
        // This loop is used to recurse over all subscopes,
        // while $sco < $this->p_scope
        //
        if( !tausch_iter_is_ok( iter ) )
        {
            // iterator became invalid
            rv = false;
            continue;   // the while cycle handles the exit
        }
        if( tausch_iter_is_end( iter ) )
        {
            if( sco == iter->scope )
            {
                // we do not step over end
                // this also handles situation where ->p_scope == 0.
                rv = false;
                continue;   // the while cycle handles the exit
            }
            else if( tausch_iter_is_eof( iter ) )
            {
                rv = false;
                continue;   // the while cycle handles the exit
            }
            else
            {
                // we exit from any subscopes
                iter->scope -= 1;
            }
        }
        if( tausch_iter_is_scope( iter ) )
        {
            // we skip over the scope
            iter->scope += 1;
        }
        // advance the iterator
        iter->idx = iter->next;
        iter->val = iter->next;

        // decode tag
        tag = tausch_iter_decode_vluint( iter );
        if( !tausch_iter_is_ok( iter ) )
        {
            return false;   // the iterator became invalid
        }
        iter->lc = tag & 3;
        iter->tag = tag >> 2;

        if( tausch_iter_is_end( iter ) )
        {
            // end of scope
            // we stay at the END marker
            // when tag part of the END is not 0, then it is also end of file
            iter->vlen = 0;
            iter->tag = iter->tag != 0 ? 1 : 0;
            iter->vlen = 0;
            iter->val = NULL;
            rv = sco < iter->scope;
            continue;   // the while cycle handles the exit
        }

        // decode length
        len = 0;
        if( iter->lc == 2 )
        {
            len = tausch_iter_decode_vluint( iter );
            if( !tausch_iter_is_ok( iter ) )
            {
                return false;   // the iterator became invalid
            }
        }
        iter->vlen = len;

        // verify length, end of buffer e.t.c
        if( len > 0 )
        {
            if( ( (iter->next + len) <= iter->next) ||
                ( (iter->next + len) > iter->ebuf) )
            {
                // the buffer overflow is happening
                iter->val = NULL;
                iter->ebuf = NULL;
                return false;   // the iterator became invalid
            }
            iter->val = iter->next;
            iter->next += len;
        }
        else
        {
            // there value is NULL
            iter->val = NULL;
        }
    }
    while( rv && (sco < iter->scope) );

    return rv;
}

/**
 * Enter the iterator into subscope if the iterator is at the beginning of scope.
 * Iterator stays to position and becomes clean.
 *
 * @param iter : tausch_iter_t* - the iterator
 * @return bool - true if succeeded false if failed
 */
bool tausch_iter_enter_scope( tausch_iter_t *iter )
{
    bool rv = tausch_iter_is_ok( iter );
    rv = rv && tausch_iter_is_scope( iter );
    if( rv )
    {
        iter->idx = iter->next;
        iter->val = iter->next;
        iter->lc = 0;
        iter->scope += 1;
    }
    return rv;
}

/**
 * Exit the iterator from current scope.
 * It does advance iterator to the next element but keeps it clean.
 *
 *
 * @param iter : tausch_iter_t* - the iterator
 * @return bool - true on success, flase on serious failure
 */
bool tausch_iter_exit_scope( tausch_iter_t *iter )
{
    bool rv = tausch_iter_is_ok( iter );
    while( rv && (!tausch_iter_is_end( iter )) )
    {
        rv = tausch_iter_next( iter );
    }
    if( !tausch_iter_is_ok( iter ) )
    {
        rv = false;
    }
    else if( tausch_iter_is_eof( iter ) )
    {
        // stay at eof
        rv = true;
    }
    else if( iter->scope == 0 )
    {
        // eos at scope 0 is disabled
        rv = false;
        iter->ebuf = NULL;   // the iterator is very broken
    }
    else if( tausch_iter_is_end( iter ) )
    {
        iter->idx = iter->next;
        iter->val = iter->next;
        iter->lc = 0;
        iter->scope -= 1;
        rv = true;
    }
    else
    {
        rv = false;
    }
    return rv;
}

/**
 * Advance the iterator to the next stuffing in the scope or next element
 * after end of scope or to the EOF
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_iter_go_to_stuffing( tausch_iter_t *iter )
{
    while( tausch_iter_next( iter ) )
    {
        if( tausch_iter_is_stuffing( iter ) )
        {
            // stuffing has ben found
            return true;
        }
    }

    if( !tausch_iter_is_ok( iter ) )
    {
        return false;
    }
    else if( tausch_iter_is_eof( iter ) )
    {
        return true;   // EOF
    }
    return false;   // EOS
}

/**
 * Advance the iterator to the next tag in the scope.
 *
 * @arg tag - the tag that is looked for
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed
 */
bool tausch_iter_go_to_tag( tausch_iter_t *iter, size_t tag )
{
    while( tausch_iter_next( iter ) )
    {
        if( iter->tag == tag )
        {
            return true;
        }
    }
    return false;
}

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
{
    size_t rv = vlen;
    rv += tausch_vluint_len( tag << 2 );
    rv += vlen > 0 ? tausch_vluint_len( vlen ) : 0;
    return rv;
}

/**
 * Calculate the amount of value field length avialablte when for TLV the total space is memlen
 *
 * @param tag : size_t - the tag id
 * @param memlen : size_t - the memory space available
 * @return size_t amount of value length available, memlen if not possible at all
 */
size_t tausch_tlv_vlen( size_t tag, size_t memlen )
{
    size_t rv = 0;
    size_t tl = tausch_vluint_len( tag << 2 );
    if( tl > memlen ) return memlen;
    if( tl == memlen ) return 0;
    size_t lv = 0;

    do
    {
        lv = tausch_vluint_len( memlen - tl - lv );
        rv = memlen - tl - lv;
    }
    while( (memlen - tl - lv) != rv );

    return rv;
}

/**
 * Overwrite the iterator memspace between idx and next, including tag, len and value.
 * It does not take much care of what is in the message already written.
 *
 * When exact is false then the remainder of space is padded with stuffing. When exact
 * is true then the iterator memspace shall be exactly aligned to fit the entire TLV.
 * In case of exact is false, then also less than len may be written. You need to verify
 * if the iter->vlen is with right amount after writing.
 *
 * When iterator is pointing at eof, it will allocate enough memory for the tlv if possible.
 *
 * When value is NULL, and length is 0 a null item will be written. It is same as single byte boolean.
 * When value is NULL and length is >0 all value fields are replaced with 0x00
 *
 *
 * @param iter : tausch_iter_t* - the prepared iterator
 * @param buf : uint8_t* - pointer to the buffer
 * @param len : size_t - amount of data in the value field
 * @param exact : bool - how to fit into memory,
 * @return size_t - amount of bytes written. 0 on error.
 */
static size_t tausch_iter_overwrite( tausch_iter_t *iter, size_t tag, uint8_t *buf, size_t len, bool exact )
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( tausch_iter_is_scope( iter ) ) return 0;

    tausch_iter_t tm = *iter;   // temporary iterator for rollback

    size_t tlvlen = tausch_tlv_size( tag, len );
    if( tausch_iter_is_eof( iter ) )
    {
        // iterator is at the eof, allocate enough memory
        if( ( (iter->idx + tlvlen + 1) >= iter->ebuf) || ( (iter->idx + tlvlen + 1) < iter->idx) )
        {
            // buffer overflow would happen
            if( (!exact) && ( (iter->ebuf - 1) > iter->idx) )
            {
                size_t memlen = iter->ebuf - iter->idx - 1;
                len = tausch_tlv_vlen( tag, memlen );
                if( len >= memlen ) return 0;   // was not able to find smaller len
                tlvlen = tausch_tlv_size( tag, len );
            }
            else
            {
                return 0;   // no exact writing possible
            }
        }
        iter->next = iter->idx + tlvlen;
        tausch_iter_format( iter );   // write next T7 into message
    }
    else if( tausch_iter_is_end( iter ) )
    {
        // if the iterator is at end then we do not overwrite
        return 0;
    }
    size_t memlen = iter->next - iter->idx;   // now, find the available memory based on iterator
    if( exact && (tlvlen != memlen) )
    {
        *iter = tm;   // we can rollback here
        return 0;   // the input data does not match exactly
    }

    if( (!exact) && (tlvlen > memlen) )
    {
        // we need to recalculate the len since not all is fitting in
        len = tausch_tlv_vlen( tag, memlen );
        if( len == memlen )
        {
            // it is not possible to write even tag
            *iter = tm;
            return 0;
        }
        tlvlen = memlen;
    }

    iter->next = iter->idx;
    iter->val = iter->idx;

    // encode TLV

    tag <<= 2;
    if( len > 0 ) tag += 2;

    bool ok = true;
    ok = ok && tausch_iter_encode_vluint( iter, tag );
    if( len > 0 )
    {
        ok = ok && tausch_iter_encode_vluint( iter, len );
        if( !ok )
        {
            iter->ebuf = NULL;   // we have messed up the message
            return 0;
        }
        if( buf == NULL )
        {
            memset( iter->next, 0x00, len );
        }
        else
        {
            memcpy( iter->next, buf, len );
        }
        iter->next += len;
    }

    // fill remainder with stuffing
    if( ok && (tlvlen < memlen) )
    {
        iter->idx = iter->next;
        iter->val = NULL;
        iter->next += memlen - tlvlen;
        iter->lc = 0;

        ok = ok && tausch_iter_erase( iter );

        tausch_iter_t si = *iter;
        while( tausch_iter_next(&si) && tausch_iter_is_stuffing(&si) )
        {
            // we have turned a part of item to be stuffing, if stuffing follows
            // then collapse the stuffings into one bigger stuffing
            // next returns false on eof
        }

        if( tausch_iter_is_eof(&si) )
        {
            // si idx is on eof, we move the eof to the beginning of iter
            tausch_format_buf(iter->idx);
            memlen = iter->idx - tm.idx;
        }
        else if( si.idx > iter->next )
        {
            // si idx is on something else,
            iter->next = si.idx;
            ok = ok && tausch_iter_erase( iter );
        }
        else
        {
            // it is the only stuffing
        }
    }

    if( !ok )
    {
        iter->ebuf = NULL;   // we have messed up the message
        return 0;
    }


    // advance iter to the first written element
    *iter = tm;
    iter->next = iter->idx;
    iter->val = iter->idx;
    iter->lc = 0;
    iter->vlen = 0;
    if( ! tausch_iter_next( iter ) )
    {
        // what ? failed ??
        iter->ebuf = NULL;
        return 0;
    }

    return memlen;
}

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
{
    if( !tausch_iter_is_ok( iter ) ) return false;
    if( tausch_iter_is_clean(iter) )
    {
        // writing clean iter is disabled, it must point to at least to eof
        return false;
    }
    if( len == 0 )
    {
        return false;
    }
    if( tausch_iter_is_eof( iter ) )
    {
        return tausch_iter_overwrite( iter, 0, NULL, tausch_tlv_vlen(0,len), true ) > 0;
    }
    len = iter->next - iter->idx;
    return tausch_iter_overwrite( iter, 0, NULL, tausch_tlv_vlen(0,len), true ) > 0;
}

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
{
    if( !tausch_iter_is_ok( iter ) ) return false;
    if( !tausch_iter_is_complete( iter ) ) return false;
    if( tausch_iter_is_end( iter ) ) return false; // end or eof
    if( tausch_iter_is_scope( iter ) )
    {
        tausch_iter_t tm = *iter;
        // advance the temporary iterator at the end of current scope
        if( ! tausch_iter_exit_scope( &tm ) ) return false;
        // idx is now at the end of eos also next is at the end
        tm.idx = iter->idx;
        *iter = tm;
    }
    return tausch_iter_write_stuffing( iter, iter->next - iter->idx );
}

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
{
    if( !tausch_iter_is_ok( iter ) ) return false;
    if( tausch_iter_is_complete( iter ) && (! tausch_iter_is_stuffing(iter)) && (!tausch_iter_is_eof(iter)) ) return false;   // scope can only be appended
    if( iter->scope == ~(typeof(iter->scope))0 ) return false;   // no more scopes can be added

    // first write it as boolean true
    bool rv = tausch_iter_write_bool( iter, tag, NULL );
    if( (!rv) || (! tausch_iter_is_ok(iter)) ) return false;
    // then add to it the collection flag
    *iter->idx &= ~3;
    *iter->idx += 1;
    iter->lc = 1;
    iter->val = NULL;
    iter->scope += 1;
    return true;
}

/**
 * Close lastly open scope
 *
 * @arg iter - the iterator where to append the scope
 *
 * @return false on failure
 * @return true on success
 */
bool tausch_iter_write_end( tausch_iter_t *iter )
{
    if( !tausch_iter_is_ok( iter ) ) return false;
    if( tausch_iter_is_complete( iter ) && (! tausch_iter_is_stuffing(iter)) && (!tausch_iter_is_eof(iter)) ) return false;   // end of scope can only be appended
    if( iter->scope == 0 ) return false;   // nothing to close

    // first write it as single byte tag 0
    size_t rv = tausch_iter_overwrite( iter, 0, NULL, 0, false );
    if( (rv == 0) || (! tausch_iter_is_ok(iter)) ) return false;
    // then turn it to eos
    *iter->idx = 3;
    iter->val = NULL;
    iter->lc = 3;
    iter->scope -= 1;
    iter->tag = ~0;
    return true;
}

/**
 * Read the iterator value field as BOOL
 *
 * @return true if the read was successful
 * @return false if the read was unsuccessful
 */
bool tausch_iter_read_bool( tausch_iter_t *iter, bool *value )
{
    if( !tausch_iter_is_ok( iter ) ) return false;
    if( !tausch_iter_is_complete( iter ) ) return false;
    if( value == NULL ) return false;
    if( tausch_iter_is_stuffing( iter ) )
    {
        *value = false;
        return true;
    }
    if( iter->vlen == 0 )
    {
        *value = true;
        return true;
    }
    else for( int i = 0; i < iter->vlen; i++ )
    {
        if( iter->val[i] != 0 )
        {
            *value = true;
            return true;
        }
    }
    *value = false;
    return true;
}

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
{
    return tausch_iter_write_typX( iter, tag, (uint8_t*)value, value ? sizeof(bool) : 0 ) > 0;
}

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
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( value == NULL ) return 0;
    if( iter->vlen != len ) return 0;
    memcpy( (void*)value, (void*)iter->val, len );
    return len;
}

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
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( value == NULL ) return 0;
    if( iter->vlen > value->len ) return 0;

    memcpy( (void*)value->buf, (void*)iter->val, iter->vlen );
    memset( value->buf + iter->vlen, 0, value->len - iter->vlen );
    return iter->vlen;
}


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
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( !tausch_iter_is_complete( iter ) ) return 0; // only complete iterator fits

    if( (value != NULL) && (len>0) )
    {
        // full tlv will be written
        if( tausch_iter_is_eof( iter ) )
        {
            // adding to the end of message
            return (tausch_iter_overwrite( iter, tag, value, len, true ) > 0 ? iter->vlen : 0);
        }
        else
        {
            if( tausch_iter_is_stuffing( iter ) )
            {
                // adding into beginning of stuffing and keeping remaining as stuffing
                tausch_iter_t tm = *iter;
                size_t rv = (tausch_iter_overwrite( iter, tag, value, len, false ) > 0 ? iter->vlen : 0);
                if( (rv == 0) || (iter->vlen != len) )
                {
                    // turn back the area to iter
                    *iter = tm;
                    tausch_iter_erase( iter );
                    return 0;
                }
                return rv;
            }
            // else we overwrite existing item
            if( iter->tag != tag ) return false;   // the tag must match
            if( iter->lc == 0 )
            {
                // it is tag only boolean. value true
                if( *value ) return true;   // overwriting with same data
                return tausch_iter_erase( iter );   // turn the item into stuffing
            }
            // it is existing full tlv item with matching tag, overwrite also len must match
            return (tausch_iter_overwrite( iter, tag, value, len, true ) > 0 ? iter->vlen : 0);
        }
    }
    // tag only boolean true value or nulled memory area will be written
    return ((tausch_iter_overwrite( iter, tag, value, len, false ) > 0) && (iter->vlen == 0) ? 1 : iter->vlen);
}

/**
 * Write the blob into the value field.
 * Less data can be written from blob buffer, look for the iter->vlen, how much was taken
 *
 * @arg iter - the iterator
 * @arg tag - the tag value
 * @arg value - the pointer to blob
 *
 * @return 0 on failure
 * @return size written from blob data on success
 */
size_t tausch_iter_write_blob( tausch_iter_t *iter, size_t tag, tausch_blob_t *value )
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( !tausch_iter_is_complete( iter ) ) return 0;   // only complete iterator fits
    if( value == NULL ) return 0;   // the value must be given
    if( value->len == 0 ) return 0;   // the length shall be more than 0

    // full tlv will be written
    if( tausch_iter_is_eof( iter ) )
    {
        // adding to the end of message
        return tausch_iter_overwrite( iter, tag, value->buf, value->len, false ) > 0 ? iter->vlen : 0;
    }
    else
    {
        if( tausch_iter_is_stuffing( iter ) )
        {
            // adding into beginning of stuffing and keeping remaining as stuffing
            tausch_iter_t tm = *iter;
            size_t rv = tausch_iter_overwrite( iter, tag, value->buf, value->len, false );
            if( rv == 0 )
            {
                // turn back the area to iter
                *iter = tm;
                tausch_iter_erase( iter );
                return 0;
            }
            return iter->vlen;
        }
        // else we overwrite existing item
        if( iter->tag != tag ) return 0;   // the tag must match
        if( iter->lc == 0 )
        {
            // it is tag only boolean. value true
            return 0;   // does not match the blob criteria !
        }
        // it is existing full tlv item with matching tag, overwrite
        return (tausch_iter_overwrite( iter, tag, value->buf, value->len, false ) > 0 ? iter->vlen : 0);
    }
}

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
{
    if( value == NULL ) return 0;

    tausch_blob_t tmp = { .buf = (uint8_t*)value };
    tmp.len = strlen( value );

    return tausch_iter_write_blob( iter, tag, &tmp );
}

/**
 * Get the length of the TLV value field
 */
size_t tausch_iter_vlen( tausch_iter_t *iter )
{
    if( !tausch_iter_is_ok( iter ) ) return 0;
    if( iter->val == NULL ) return 0;
    if( tausch_iter_is_complete( iter ) )
    {
        return iter->vlen;
    }
    return 0;
}

/**
 * Produce result blob that references only slice of orig blob.
 *
 * @param result : tausch_blob_t* - the memory field of produced blob reference
 * @param orig : tausch_blob_t* - the memory fileld of original blob reference
 * @param offset : size_t - the offset in the original blob
 * @param len : size_t - the amount of bytes in resulting blob reference
 * @return
 */
tausch_blob_t* tausch_blob_slice( tausch_blob_t *result, tausch_blob_t *orig, size_t offset, size_t len )
{
    *result = *orig;
    if( ( (offset + len) > offset) && (offset <= result->len) )
    {
        // no memory access fault would happen
        result->buf += offset;
        result->len -= offset;
        result->len = result->len > len ? len : result->len;
    }
    else
    {
        result->len = 0;
    }
    return result;
}

