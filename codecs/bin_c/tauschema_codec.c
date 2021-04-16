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

/**
 * Decode from binary buffer the variable length unsigned integer
 *
 * @arg buf - pointer to the buffer
 * @arg idx - pointer to the buffer index
 *
 * @return the decoded tag value, idx will be updated
 *
 */
size_t tausch_decode_vluint( tausch_iter_t *iter )
{
    size_t rv = 0;
    size_t x = 0;
    uint32_t s = 0;
	do
	{
	    if( iter->ebuf == NULL )
	    {
	        return -1;
	    }
	    if( iter->next >= iter->ebuf )
	    {
	        iter->ebuf = NULL;
	        return -1;
	    }
		x = (size_t)(*iter->next) & 0x7f;
		if( (s+1) < (sizeof(size_t)*8) )
		{
			// we do not support bigger numbers for vluint
			rv |= x << s;
			s += 7;
		}
		iter->next += 1;
	}
	while( (x & 0x80) & ( s < (sizeof(size_t)*8)) );

	return rv;
}

/**
 * Return 1 if iterator is broken, otherwise 0
 */
uint8_t tausch_iter_broken( tausch_iter_t *iter )
{
    return iter->ebuf == NULL;
}

/**
 * Decodes from binary buffer next TLV item and stores info
 * inside iter element. It does not skip over stuffing.
 *
 * @return 0 if the iteration was successful
 * @return 1 if the iteration failed, and iterator is unusable
 */
uint32_t tausch_decode_next( tausch_iter_t *iter )
{
    size_t tag = 0;
    size_t len = 0;
    iter->idx = iter->next;
    while( 1 )
    {
        tag = tausch_decode_vluint( iter );
        if( tausch_iter_broken( iter ) ) return 1;
        if( tag & 3 == 3 )
        {
            // end of scope
            if( iter->scope == 0 )
            {
                iter->ebuf = NULL;
                return 1;
            }
            iter->scope -= 1;
            continue;
        }
        break;
    }
    len = 0;
    if( tag & 1 == 1 )
    {
        // collection or variadic tag
        iter->scope += 1;
    }
    if( tag & 2 == 2 )
    {
        len = tausch_decode_vluint( iter );
        if( tausch_iter_broken( iter ) ) return 1;
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
            return 1;
        }
        iter->val = iter->next;
        iter->next += len; // idx may now equel to exbuf
    }
    else
    {
        iter->val = NULL;
    }

    return 0;
}

/**
 * Advance the iterator to the element next item on the same scope
 *
 * @return 0 if the iteration was successful
 * @return 1 if the iteration failed, and iterator is unusable
 */
uint32_t tausch_decode_to_eoscope( tausch_iter_t *iter )
{
    uint16_t scope = iter->scope;
    do
    {
        tausch_decode_next( iter );
        if( tausch_iter_broken( iter ) ) return 1;
    }
    while( iter->scope > scope );
    return 0;
}


/**
 * Advance the iterator to the element next item after end of current scope
 *
 * @return 0 if the iteration was successful
 * @return 1 if the iteration failed, and iterator is unusable
 */
uint32_t tausch_decode_to_eoscope( tausch_iter_t *iter )
{
    uint16_t scope = iter->scope;
    if( scope == 0 ) return 0;
    do
    {
        tausch_decode_next( iter );
        if( tausch_iter_broken( iter ) ) return 1;
    }
    while( iter->scope >= scope );
    return 0;
}

/**
 * Advance the iterator to the next stuffing in the scope or next element
 * after end of scope
 *
 * @return 0 if the iteration was successful
 * @return 1 if the iteration failed, and iterator is unusable
 */
uint32_t tausch_decode_to_stuffing( tausch_iter_t *iter )
{
    uint16_t scope = iter->scope;
    do
    {
        tausch_decode_next( iter );
        if( tausch_iter_broken( iter ) ) return 1;
        if( (iter->tag == 0) && (iter->lc != 1) )
        {
            // stuffing has been found
            return 0;
        }
    }
    while( iter->scope >= scope );
    return 0;
}

/**
 * Turn the element pointed by iterator into stuffing
 */
void tausch_overwrite_with_stuffing( tausch_iter_t *iter )
{
    if( taush_iter_broken( iter ) ) return;
    size_t totlen = iter->next - iter->idx;
    if( totlen < 1 ) return;
    if( totlen == 1 )
    {
        iter->idx[0] = 0x00;
    }
    else
    {
        tausch_iter_t tmp = *iter;
        tausch_encode_stuffing( tmp, totlen );
    }
}

