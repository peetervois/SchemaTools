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


#include "tauschema_codec.h"
#include "string.h"


/**
 * Return true if iterator is ok false if it is broken
 */
bool tausch_iter_is_ok( tausch_iter_t *iter )
{
    bool notok = false;
    notok |= iter->ebuf == NULL; // there is no buffer pointed
    notok |= iter->idx == NULL; // idx is pointing to nothing
    notok |= iter->idx >= iter->ebuf; // index is out of buffer or end of buffer
    notok |= iter->next == NULL; // next is pointing to nothing
    notok |= iter->next > iter->ebuf; // next is pointing out of buffer
    notok |= (iter->next == iter->idx) && (iter->val != iter->next); // iter->val has incorrect value
    return ! notok;
}

/**
 * Return true if the iterator is complete
 */
bool tausch_iter_is_complete( tausch_iter_t *iter )
{
    // we do not check if iter is ok here, because that would reduce dimensions
    return iter->val != iter->next;
}

/**
 * Return true if the value is null
 */
bool tausch_iter_is_null( tausch_iter_t *iter )
{
    return iter->val == NULL;
}

/**
 * Return true if the iterator is clean for write
 */
bool tausch_iter_is_clean( tausch_iter_t *iter )
{
    if( ! tausch_iter_is_ok( iter ) ) return false;
    return (iter->next == iter->idx) && (iter->val == iter->next);
}

/**
 * Return size of the stuffing when the iter is stuffing, otherwise 0
 */
size_t tausch_iter_is_stuffing( tausch_iter_t *iter )
{
    if( ! tausch_iter_is_ok( iter ) ) return 0;
    if( iter-> tag != 0 ) return 0;
    return iter->next - iter->idx;
}

/**
 * Decode from binary buffer the variable length unsigned integer
 *
 * @arg iter - the iterator from where to read
 *
 * @return the decoded tag value, or ~0 (all bits set) on failure
 *
 */
size_t tausch_decode_vluint( tausch_iter_t *iter )
{
    size_t rv = 0;
    size_t x = 0;
    uint32_t s = 0;
    if( iter->next != iter->val ) return ~0;
	do
	{
	    if( ! tausch_iter_is_ok( iter ) ) return ~0;
	    if( iter->next >= iter->ebuf )
	    {
	        iter->ebuf = NULL;
	        return ~0;
	    }
		x = (size_t)(*iter->next);
		if( (s+1) < (sizeof(size_t)*8) )
		{
			// we do not support bigger numbers for vluint
			rv |= (x & 0x7f) << s;
			s += 7;
		}
		iter->next += 1;
	}
	while( ((x & 0x80) == 0x80) && ( s < (sizeof(size_t)*8)) );
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
bool tausch_encode_vluint( tausch_iter_t *iter, size_t val )
{
    size_t x = val;
    uint8_t b = 0;
    if( tausch_iter_is_complete(iter) ) return false; // the iterator is complete already
    do
    {
        if( ! tausch_iter_is_ok( iter ) ) return false;
        if( x & (~0x7f) )
            b = 0x80;
        else
            b = 0x00;
        b |= x & 0x7f;
        *(iter->next++) = b;
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
    unsigned int *x = (unsigned int *)&val;
    size_t rv = 0; // now: rv is number of msbits zeroed
    for( int i = (sizeof(size_t)/sizeof(unsigned int)-1); i >= 0; i-- )
    {
        if( x[i] == 0 )
        {
            rv += 32;
        }
        else
        {
            rv += __builtin_clz( x[i] );
            break;
        }
    }
    rv = (sizeof(size_t)<<3) - rv; // now: rv is number of significant bits
    rv = (rv+6) / 7; // now: rv is number of bytes
    return rv;
}

/**
 * Decodes from binary buffer next TLV item and stores info
 * inside iter element. It does not skip over stuffing.
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_next( tausch_iter_t *iter )
{
    size_t tag = 0;
    size_t len = 0;
    iter->idx = iter->next;
    iter->val = iter->next;
    while( 1 )
    {
        tag = tausch_decode_vluint( iter );
        if( ! tausch_iter_is_ok( iter ) ) return false;
        if( (tag & 3) == 3 )
        {
            // end of scope
            if( iter->scope == 0 )
            {
                iter->ebuf = NULL;
                return false;
            }
            iter->scope -= 1;
            continue;
        }
        break;
    }
    len = 0;
    if( (tag & 1) == 1 )
    {
        // collection or variadic tag
        iter->scope += 1;
    }
    if( (tag & 2) == 2 )
    {
        len = tausch_decode_vluint( iter );
        if( ! tausch_iter_is_ok( iter ) ) return false;
    }
    iter->lc = tag & 3;
    iter->tag = tag >> 2;
    iter->vlen = len;
    if( len > 0 )
    {
        if( ((iter->next + len) <= iter->next) || ((iter->next + len) > iter->ebuf) )
        {
            iter->val = NULL;
            iter->ebuf = NULL;
            return false;
        }
        iter->val = iter->next;
        iter->next += len; // idx may now equel to exbuf
    }
    else
    {
        iter->val = NULL;
    }

    return true;
}

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
bool tausch_write_next( tausch_iter_t *iter )
{
    if( ! tausch_iter_is_ok( iter ) ) return false;
    iter->idx = iter->next;
    iter->val = iter->idx;
    iter->vlen = 0;
    iter->tag = 0;
    iter->lc = 0;
    // note: iter->scope stays as it is
    return true;
}

/**
 * Advance the iterator to the element next item on the same scope
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_to_next( tausch_iter_t *iter )
{
    uint16_t scope = iter->scope;
    do
    {
        tausch_decode_next( iter );
        if( ! tausch_iter_is_ok( iter ) ) return false;
    }
    while( iter->scope > scope );
    return true;
}


/**
 * Advance the iterator to the element next item after end of current scope
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_to_eoscope( tausch_iter_t *iter )
{
    uint16_t scope = iter->scope;
    if( scope == 0 ) return true;
    do
    {
        tausch_decode_next( iter );
        if( ! tausch_iter_is_ok( iter ) ) return false;
    }
    while( iter->scope >= scope );
    return true;
}

/**
 * Advance the iterator to the next stuffing in the scope or next element
 * after end of scope
 *
 * @return true if the iteration was successful
 * @return false if the iteration failed, and iterator is unusable
 */
bool tausch_decode_to_stuffing( tausch_iter_t *iter )
{
    uint16_t scope = iter->scope;
    do
    {
        tausch_decode_next( iter );
        if( ! tausch_iter_is_ok( iter ) ) return false;
        if( (iter->tag == 0) && (iter->lc != 1) && (iter->scope == scope) )
        {
            // stuffing has been found
            return true;
        }
    }
    while( iter->scope >= scope );
    return false;
}

/**
 * Turn the element pointed by iterator into stuffing.
 * If the iterator is incomplete, then it creates.
 *
 * @arg len - the amount of bytes to write
 *
 * @return true if the stuffing was successful
 * @return false if the stuffing was unsuccessful
 */
bool tausch_write_stuffing( tausch_iter_t *iter, size_t len )
{
    // FIXME: check if the usage of this method matches what it does, like what happens with idx
    if( ! tausch_iter_is_ok( iter ) ) return false;
    size_t totlen = iter->next - iter->idx;
    if( iter->next == iter->val )
    {
        // the iterator is incomplete
        totlen =len;
    }
    if( totlen < 1 ) return true;
    if( ((iter->idx + totlen) <= iter->idx) || ((iter->idx + totlen) > iter->ebuf) ) return true;
    iter->next = iter->idx;
    iter->val = iter->next;
    size_t tlv = 0;
    if( totlen == 1 )
    {
        if( ! tausch_encode_vluint( iter, tlv ) ) return false;
        iter->val = NULL;
    }
    else
    {
        tlv = 2;
        if( ! tausch_encode_vluint( iter, tlv ) ) return false;
        tlv = totlen - (size_t)iter->next + (size_t)iter->idx;
        size_t lv  = tausch_vluint_len( tlv ) , xv = tlv;
        do { xv =tlv - lv; lv = tausch_vluint_len( xv ); } while( (xv+lv) != tlv );
        if( ! tausch_encode_vluint( iter, xv ) ) return false;
        iter->val = iter->next;
        tlv = totlen - (size_t)iter->next + (size_t)iter->idx;
        memset( iter->next, 0, tlv );
    }
    iter->next = iter->idx + totlen;
    return true;
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
bool tausch_write_scope( tausch_iter_t *iter, size_t *tag )
{
    if( ! tausch_iter_is_ok( iter ) ) return false;
    if( tausch_iter_is_complete( iter ) ) return false; // scope can only be appended
    if( iter->scope == ~(typeof(iter->scope))0 ) return false; // no more scopes can be added
    size_t k = *tag;
    k <<= 2;
    k += 1;
    if( ! tausch_encode_vluint( iter, k ) ) return false;
    iter->val = NULL;
    iter->lc = 1;
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
bool tausch_write_end( tausch_iter_t *iter )
{
    if( ! tausch_iter_is_ok( iter ) ) return false;
    if( tausch_iter_is_complete( iter ) ) return false; // end of scope can only be appended
    if( iter->scope <= 0 ) return false; // nothing to close
    size_t k = 3;
    if( ! tausch_encode_vluint( iter, k ) ) return false;
    iter->val = NULL;
    iter->lc = 3;
    iter->scope -= 1;
    return true;
}

/**
 * Read the iterator value field as BOOL
 *
 * @return true if the read was successful
 * @return false if the read was unsuccessful
 */
bool tausch_read_bool( tausch_iter_t *iter, bool *value )
{
    if( ! tausch_iter_is_ok( iter ) ) return false;
    if( value == NULL ) return false;
    if( iter->vlen == 0 )
    {
        *value = 1;
        return true;
    }
    else for( int i=0; i < iter->vlen; i++ )
    {
        if( iter->val[i] != 0 )
        {
            *value = 1;
            return true;
        }
    }
    *value = 0;
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
 * @return true if the read was successful
 * @return false if the read was unsuccessful
 */
bool tausch_write_bool( tausch_iter_t *iter, size_t tag, bool *value )
{
    if( ! tausch_iter_is_ok( iter ) ) return false;
    bool val = true;
    if( value != NULL ) val = *value;
    if( ! tausch_iter_is_complete( iter ) )
    {
        // Note if you want to append it in non space saving way, then
        // before calling this method, do tausch_write_typX( iter, &tag, &value, 1 )
        if( ! value )
        {
            // Append the element in a space saving way.
            size_t tlv = tag << 2;
            if( ! tausch_encode_vluint( iter, tlv ) ) return false;
            iter->val = NULL;
        }
        else
        {
            // Write the item as uint8_t
            if( ! tausch_write_typX( iter, tag, (uint8_t*)&val, sizeof(uint8_t))) return false;
        }
        return true;
    }
    else
    {
        // the iterator already has been read, we are overwriting
        size_t totlen = iter->next - iter->idx;
        tausch_iter_t tm = *iter;
        tm.next = tm.idx;
        tm.val = tm.next;
        tm.ebuf = iter->next;
        size_t bytes = tausch_vluint_len( tag );
        if( totlen < bytes )
        {
            return false; // can not encode, not enough space in it
        }
        else if( totlen <= (bytes+1) )
        {
            // only true value can be encoded
           if( val == true )
           {
               size_t tlv = tag << 2;
               if( ! tausch_encode_vluint( &tm, tlv ) ) return false;
               // expected that only 1 byte will be written, appending more 0-s is ok
               while(tm.next < tm.ebuf)
               {
                   *(tm.next++) = 0x00;
               }
           }
           else // val == false
           {
               if( ! tausch_write_stuffing( &tm, totlen ) ) return false;
           }
           return true;
        }
        else
        {
            // full TLV has to be written
            size_t tlv = tag << 2;
            tlv += 2;   // encode the len too
            if( ! tausch_encode_vluint( &tm, tlv ) ) return false;
            tlv = totlen - bytes - 1;   // now: tlv is length
            if( ! tausch_encode_vluint( &tm, tlv ) ) return false;
            while( tm.next < tm.ebuf )
            {
                *(tm.next++) = val;
                val = 0;
            }
            return true;
        }
    }

    return true;
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
size_t tausch_read_typX( tausch_iter_t *iter, uint8_t *value, size_t len )
{
    if( ! tausch_iter_is_ok( iter ) ) return 0;
    if( value == NULL ) return 0;
    if( iter->vlen != len ) return 0;
    memcpy( (void*)value, (void*)iter->val, len);
    return len;
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
size_t tausch_write_typX( tausch_iter_t *iter, size_t tag, uint8_t *value, size_t len )
{
    if( ! tausch_iter_is_ok( iter ) ) return 0;
    if( tausch_iter_is_complete( iter ) )
    {
        // we are overwriting the meomry
        if( iter->tag != tag ) return 0;
        if( value != NULL )
        {
            // simple overwrite of the value
            if( iter->vlen != len ) return 0; // the length must match
            memcpy( iter->val, value, len );
            return len;
        }
        else if( len > 0 )
        {
            // we are zeroing the  value field
            if( iter->vlen != len ) return 0; // lengths must match in this case
            memset( iter->val, 0x00, len );
            return len;
        }
        else
        {
            // we are turning the item to null in place
            if( iter->vlen == 0 ) return 1; // it is already null
            tag <<= 2;
            // start new iterator for overwriting
            tausch_iter_t tm = *iter;
            tm.next = tm.idx;
            tm.val = tm.next;
            tm.ebuf = iter->next;
            tm.lc = 0;
            tm.vlen = 0;
            if( ! tausch_encode_vluint( &tm, tag ) ) return 0;
            iter->next = tm.next;
            iter->val = NULL;
            iter->vlen = 0;
            iter->lc = 0;
            // fill the remainder with stuffing
            size_t stlen = tm.ebuf - tm.next;
            tm.idx = tm.next;
            if( ! tausch_write_stuffing( &tm, stlen ) ) return 0;
            return 1;
        }
    }
    else if ( ! tausch_iter_is_clean( iter ) )
    {
        return 0; // if the iter is not clean we can not write
    }
    else
    {
        // we are appending to the stream
        iter->tag = tag;
        tag <<= 2;
        if( len > 0 ) tag += 2;
        iter->lc = tag & 3;
        // write the tag
        if( ! tausch_encode_vluint(iter, tag) ) return 0;
        if( len == 0 )
        {
            // null value was requested
            iter->val = NULL;
            iter->vlen = 0;
            return 1;
        }
        // write the len
        if( ! tausch_encode_vluint(iter, len) ) return 0;
        // write the value
        if( ((iter->next + len) <= iter->next) || ((iter->next + len) > iter->ebuf) ) return 0; // overflow
        if( value != NULL )
        {
            memcpy( iter->next, value, len );
        }
        else
        {
            memset( iter->next, 0x00, len );
        }
        iter->next += len;
        iter->vlen = len;
        return len;
    }
}

/**
 * Read the iterator value field as UINT-X
 *
 * @return 0 unsuccessful
 * @return number of bytes read out
 */
size_t tausch_read_blob( tausch_iter_t *iter, tausch_blob_t *value )
{
    if( ! tausch_iter_is_ok( iter ) ) return 0;
    if( value == NULL ) return 0;
    if( value->len < iter->vlen ) return 0;
    memcpy( (void*)value, (void*)iter->val, iter->vlen);
    return iter->vlen;
}

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
size_t tausch_write_blob( tausch_iter_t *iter, size_t tag, tausch_blob_t *value )
{
    if( value == NULL )
    {
        return tausch_write_typX( iter, tag, NULL, 0 );
    }
    return tausch_write_typX( iter, tag, value->buf, value->len );
}

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
size_t tausch_write_utf8( tausch_iter_t *iter, size_t tag, char *value )
{
    if( value == NULL )
    {
        return tausch_write_typX( iter, tag, NULL, 0 );
    }
    if( ! tausch_iter_is_ok( iter ) ) return 0;
    size_t stlen = strlen( value );
    if( tausch_iter_is_complete( iter ) )
    {
        if( tausch_iter_is_null( iter ) ) return 0; // the iter is null
        if( stlen > iter->vlen ) return 0; // the string does not fill in
        memcpy( iter->val, value, stlen );
        memset( iter->val + stlen, 0x00, iter->vlen-stlen );
        return iter->vlen;
    }
    else
    {
        return tausch_write_typX( iter, tag, (uint8_t*)value, stlen );
    }
}


/**
 * Get the length of the TLV value field
 */
size_t tausch_iter_vlen( tausch_iter_t *iter )
{
    if( ! tausch_iter_is_ok( iter ) ) return 0;
    return iter->vlen;
}



