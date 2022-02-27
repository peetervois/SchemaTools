/*
 * TauSchema Check C
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

#include "tauschema_check.h"
#include "string.h"
#include <stdarg.h>

bool tausch_schema_init( tausch_schema_t *schema, const uint8_t *tlv, tsch_size_t len )
{
    tausch_iter_t iter;

    (void)tausch_iter_init( &iter, (uint8_t*)tlv, len );
    schema->rows.buf = NULL;
    schema->rows.len = 0;
    schema->names.buf = NULL;
    schema->names.len = 0;
    schema->descriptions.buf = NULL;
    schema->descriptions.len = 0;

    while( tausch_iter_next( &iter ) )
    {
        switch( iter.tag )
        {
            case 1:
            {
                // names
                schema->names.buf = &iter.buf[iter.val];
                schema->names.len = tausch_iter_vlen( &iter );
                break;
            }
            case 2:
            {
                // descriptions
                schema->descriptions.buf = &iter.buf[iter.val];
                schema->descriptions.len = tausch_iter_vlen( &iter );
                break;
            }
            case 3:
            {
                // rows
                schema->rows.buf = &iter.buf[iter.val];
                schema->rows.len = tausch_iter_vlen( &iter );
                break;
            }
            default:
            // skip the item that we do not know about
                break;
        }
    }
    return tausch_iter_is_ok( &iter );
}

tsch_size_t tausch_schema_name_n( tausch_schema_t *schema, char *name_x )
{
    // TODO: tausch_schema_name_n is not implemented !
    return 0;
}

tsch_size_t tausch_schema_str( tausch_blob_t *strings, char *name_x, tsch_size_t name_len, tsch_size_t name_n )
{
    tsch_size_t rv = 0;
    name_x[0] = 0;
    if( name_n < strings->len )
    {
        strncpy( name_x, (char*)&strings->buf[name_n], name_len );
        strings->buf[name_len - 1] = 0;
        rv = strlen( (char*)&strings->buf[name_n] );
    }
    return rv;
}

bool tausch_flatrow_init( tausch_flatrow_t *row, tausch_schema_t *schema )
{
    row->schema = schema;
    row->item = 0;
    row->name = 0;
    row->desc = 0;
    row->ntype = TSCH_NONE;
    row->sub = 0;
    row->next = 0;
    return true;
}

bool tausch_flatrow_decode( tausch_flatrow_t *row, tsch_size_t idx )
{
    tausch_iter_t iter;

    if( idx >= row->schema->rows.len ) return false;

    tausch_iter_init( &iter, row->schema->rows.buf + idx, row->schema->rows.len - idx );

    row->item = tausch_iter_decode_vluint( &iter );
    if( row->item == TSCH_NOTHING ) return false;

    row->name = tausch_iter_decode_vluint( &iter );
    if( row->item == TSCH_NOTHING ) return false;

    row->ntype = tausch_iter_decode_vluint( &iter );
    if( row->ntype == TSCH_NOTHING ) return false;

    row->sub = tausch_iter_decode_vluint( &iter );
    if( row->sub == TSCH_NOTHING ) return false;

    row->next = tausch_iter_decode_vluint( &iter );
    if( row->next == TSCH_NOTHING ) return false;

    if( (row->schema->descriptions.buf != NULL) && (row->schema->descriptions.len > 0) )
    {
        row->desc = tausch_iter_decode_vluint( &iter );
        if( row->desc == TSCH_NOTHING ) return false;
    }

    return true;
}

bool tausch_flater_init( tausch_flater_t *flat, tausch_schema_t *schema, uint8_t *msg, tsch_size_t msg_len )
{
    if( (flat == NULL) || (schema == NULL) || (msg == NULL) || (msg_len == 0) ) return false;
    (void)tausch_iter_init( &flat->iter, msg, msg_len );
    (void)tausch_flatrow_init( &flat->row, schema );
    flat->scope = 0;
    flat->idx = TSCH_NOTHING;
    return true;
}

tausch_flater_t* tausch_flater_reset( tausch_flater_t *flat )
{
    flat->idx = TSCH_NOTHING;
    flat->scope = 0;
    tausch_flatrow_decode(&flat->row, flat->scope);
    tausch_iter_reset( &flat->iter );
    return flat;
}

#if 0
static tsch_size_t tausch_nargs( va_list argptr )
{
    tsch_size_t rv = 0;
    while( va_arg(argptr, tsch_size_t) > 0 ) rv ++;
    return rv;
}
#endif

static tausch_flater_t* v_tausch_flater_go_to( tausch_flater_t *flat, va_list argptr )
{
    tsch_size_t item;
    while( (item = va_arg( argptr, tsch_size_t )) > 0 )
    {
        if( (flat->idx > 0) && (flat->idx != TSCH_NOTHING) && (flat->row.sub > 0) && (flat->scope != flat->idx) )
        {
            // the iterator has been stopping on the scope
            // in this case we advance the scope pointer
            flat->scope = flat->idx;
            if( (flat->iter.buf != NULL) && (!tausch_iter_enter_scope( &flat->iter )) )
            {
                flat->idx = 0;   // dead end
                break;
            }
        }
        //start from the beginning of scope
        tausch_flatrow_decode( &flat->row, flat->scope );
        flat->idx = flat->row.sub;
        tausch_flatrow_decode( &flat->row, flat->idx );

        while( (flat->idx > 0) && (flat->idx != TSCH_NOTHING) )
        {
            if( flat->row.name == item )
            {
                // the item was found from schema, find it from the binary too
                if( (flat->iter.buf == NULL) || (tausch_iter_go_to_tag( &flat->iter, flat->row.item )) )
                {
                    break;
                }
                flat->idx = 0;
            }
            else
            {
                flat->idx = flat->row.next;
            }
            tausch_flatrow_decode( &flat->row, flat->idx );
        }
    }

    va_end( argptr );

    return flat;
}

tausch_flater_t* tausch_flater_go_to_donotuse( tausch_flater_t *flat, ... )
{
    va_list argptr;

    if( flat->idx == 0 ) return flat;   // the flaterator is stuck

    va_start( argptr, flat );

    return v_tausch_flater_go_to( flat, argptr );
}

static bool tausch_flater_go_to_stuffing( tausch_flater_t *flater )
{
    // special handling on going into stuffing and keeping track on the scope of flat tree.
    // look if there is stuffing in current scope
    tausch_iter_t it = flater->iter;
    bool rv = tausch_iter_go_to_stuffing( &it );
    if( rv )
    {
        // stuffing or EOF has been found in current scope
        flater->iter = it;
        // set also the flat tree index to the start of scope
        if( flater->scope > 0 )
        {
            flater->idx = flater->scope;
        }
        else
        {
            flater->idx = TSCH_NOTHING;
        }
        tausch_flatrow_decode( &flater->row, flater->scope );
    }
    else
    {
        // end of scope has been found instead
    }
    return rv;
}

tausch_flater_t* tausch_flater_next( tausch_flater_t *flat )
{
    if( flat->iter.buf == NULL )
    {
        // we do not have binary iterator, only flat tree iteration
        if( flat->idx == TSCH_NOTHING )
        {
            // we have initiated iterator
            tausch_flatrow_decode( &flat->row, flat->scope );
            flat->idx = flat->row.sub;
            tausch_flatrow_decode( &flat->row, flat->idx );
        }
        if( (flat->idx > 0) && (flat->idx != TSCH_NOTHING) )
        {
            flat->idx = flat->row.next;
            tausch_flatrow_decode( &flat->row, flat->idx );
        }
    }
    // now if we iterate with binary message too, the approach is different
    else if( !tausch_iter_next( &flat->iter ) )
    {
        flat->idx = 0;
        tausch_flatrow_decode( &flat->row, flat->idx );
    }
    else
    {
        // start search from the beginning of the scope of schema
        tausch_flatrow_decode( &flat->row, flat->scope );
        flat->idx = flat->row.sub;
        tausch_flatrow_decode( &flat->row, flat->idx );

        while( (flat->idx > 0) && (flat->idx != TSCH_NOTHING) && (flat->iter.tag != flat->row.item) )
        {
            flat->idx = flat->row.next;
            tausch_flatrow_decode( &flat->row, flat->idx );
        }
        // if the schemaitem was not found in the scope, then pidx is 0
    }
    return flat;
}

tausch_flater_t* tausch_flater_go_eof( tausch_flater_t *flat )
{
    tausch_flater_reset( flat );
    if( flat->iter.buf != NULL )
    {
        if( (!tausch_iter_exit_scope( &flat->iter )) || (!tausch_iter_is_eof( &flat->iter )) )
        {
            tausch_flater_reset( flat );
        }
    }
    return flat;
}

tausch_flater_t tausch_flater_clone( tausch_flater_t *flat )
{
    tausch_flater_t rv = *flat;
    if( (rv.idx > 0) && (rv.idx != TSCH_NOTHING) && (rv.row.sub > 0) )//&& (rv.scope != rv.idx) )
    {
        // the iterator has been stopping on the scope
        // in this case we advance the scope pointer
        rv.scope = rv.idx;
        if( (rv.iter.buf != NULL) && (!tausch_iter_enter_scope( &rv.iter )) )
        {
            rv.idx = 0;   // dead end
        }
    }
    return rv;
}

char* tausch_flater_tag_x( tausch_flater_t *flat )
{
    uint8_t *buf = flat->row.schema->names.buf;
    if( (flat->idx > 0) && (flat->idx != TSCH_NOTHING) && (buf != NULL) )
    {
        return (char*)buf + flat->row.name;
    }
    return 0;
}

static bool valconv_uint2uint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    uint64_t in = 0;
    memcpy( &in, from, fromlen );
    memcpy( to, &in, tolen );
    for( int i = tolen; i < fromlen; i++ )
    {
        if( from[i] != 0 ) return false;
    }
    return true;
}

static bool valconv_uint2sint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    uint64_t in = 0;
    memcpy( &in, from, fromlen );
    memcpy( to, &in, tolen );
    if( to[tolen - 1] >= 128 ) return false;
    for( int i = tolen; i < fromlen; i++ )
    {
        if( from[i] != 0 ) return false;
    }
    return true;
}

static bool valconv_sint2uint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    uint64_t in = 0;
    if( from[fromlen - 1] >= 128 ) return false;
    memcpy( &in, from, fromlen );
    memcpy( to, &in, tolen );
    for( int i = tolen; i < fromlen; i++ )
    {
        if( from[i] != 0 ) return false;
    }
    return true;
}

static bool valconv_sint2sint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    int64_t in = 0;
    if( from[fromlen - 1] >= 128 ) in = -1;
    memcpy( &in, from, fromlen );
    memcpy( to, &in, tolen );
    for( int i = tolen; i < fromlen; i++ )
    {
        if( (from[i] != 0) || (from[i] != 255) ) return false;
    }
    return true;
}

static bool valconv_float2float( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    double resd;
    uint64_t in;
    memcpy( &in, from, fromlen );
    if( fromlen == 4 )
    {
        resd = *(float*)&in;
    }
    else if( fromlen == 8 )
    {
        resd = *(double*)&in;
    }
    else
    {
        return false;
    }
    if( tolen == 4 )
    {
        float resf = (float)resd;
        memcpy( to, &resf, tolen );
    }
    else if( tolen == 8 )
    {
        memcpy( to, &resd, tolen );
    }
    else
    {
        return false;
    }
    return true;
}

static bool valconv_float2uint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    uint64_t res = 0;
    uint64_t in;
    memcpy( &in, from, fromlen );
    if( fromlen == 4 )
    {
        res = *(float*)&in;
    }
    else if( fromlen == 8 )
    {
        res = *(double*)&in;
    }
    else
    {
        return false;
    }
    memcpy( to, &res, tolen );
    for( int i = tolen; i < fromlen; i++ )
    {
        if( ((uint8_t*)&res)[i] != 0 ) return false;
    }
    return true;
}

static bool valconv_float2sint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    int64_t res = 0;
    uint64_t in;
    memcpy( &in, from, fromlen );
    if( fromlen == 4 )
    {
        res = *(float*)&in;
    }
    else if( fromlen == 8 )
    {
        res = *(double*)&in;
    }
    else
    {
        return false;
    }
    memcpy( to, &res, tolen );
    for( int i = tolen; i < fromlen; i++ )
    {
        if( ( ((uint8_t*)&res)[i] != 0) || ( ((uint8_t*)&res)[i] != 255) ) return false;
    }
    return true;
}

static bool valconv_uint2float( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    uint64_t res = 0;
    memcpy( (void*)&res, (void*)from, fromlen );
    if( tolen == 4 )
    {
        *(float*)to = res;
    }
    else if( tolen == 8 )
    {
        *(double*)to = res;
    }
    else
    {
        return false;
    }
    return true;
}

static bool valconv_sint2float( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    int64_t res = 0;
    if( from[fromlen - 1] >= 128 ) res = -1;
    memcpy( (void*)&res, (void*)from, fromlen );
    if( tolen == 4 )
    {
        *(float*)to = res;
    }
    else if( tolen == 8 )
    {
        *(double*)to = res;
    }
    else
    {
        return false;
    }
    return true;
}

static bool valconv_bool2bool( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    memset( to, 0, tolen );
    *to = *from;
    return true;
}

static bool valconv_bool2uint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    memset( to, 0, tolen );
    *to = *from;
    return true;
}

static bool valconv_bool2sint( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    memset( to, 0, tolen );
    *to = *from;
    return true;
}

static bool valconv_bool2float( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    memset( to, 0, tolen );
    if( tolen == 4 )
    {
        *(float*)to = *from != 0 ? 1.0 : 0.0;
    }
    else if( tolen == 8 )
    {
        *(double*)to = *from != 0 ? 1.0 : 0.0;
    }
    else
    {
        return false;
    }
    return true;
}

static bool valconv_uint2bool( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    memset( to, 0, tolen );
    for( int i = 0; i < fromlen; i++ )
    {
        if( from[i] != 0 )
        {
            *to = 1;
            break;
        }
    }
    return true;
}

static bool valconv_sint2bool( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    return valconv_uint2bool( to, tolen, from, fromlen );
}

static bool valconv_float2bool( uint8_t *to, uint8_t tolen, uint8_t *from, uint8_t fromlen )
{
    memset( to, 0, tolen );
    if( fromlen == 4 )
    {
        float f = *(float*)from;
        *to = ( (f > 0.001) || (f < -0.001)) ? 1 : 0;
    }
    else if( fromlen == 8 )
    {
        double f = *(double*)from;
        *to = ( (f > 0.001) || (f < -0.001)) ? 1 : 0;
    }
    else if( fromlen == 16 )
    {
        long double f = *(long double*)from;
        *to = ( (f > 0.001) || (f < -0.001)) ? 1 : 0;
    }
    else
    {
        return false;
    }
    return true;
}

/*
 * list of type sizes, > 99 means various lengths
 */
static uint8_t valuelengths[TSCH_UTF8] = {
    0, sizeof(bool),
    100, 1, 2, 4, 8,
    100, 1, 2, 4, 8,
    100, 4, 8
};

static bool valconv( tausch_ntype_t totyp, uint8_t *to, uint8_t tolen, tausch_ntype_t fromtyp, uint8_t *from,
    uint8_t fromlen )
{
    if( (fromlen > 8) || (tolen > 8) ) return false;
    if( (fromtyp > TSCH_FLOAT_64) || (totyp > TSCH_FLOAT_64) ) return false;
    if( ( (fromlen != valuelengths[fromtyp]) && (valuelengths[fromtyp] < 100)) ) return false;
    if( ( (tolen != valuelengths[totyp]) && (valuelengths[totyp] < 100)) ) return false;
    if( totyp >= TSCH_UTF8 )
    {
        return false;
    }
    else if( totyp >= TSCH_FLOAT )
    {
        if( fromtyp >= TSCH_UTF8 )
        {
            return false;
        }
        else if( fromtyp >= TSCH_FLOAT )
        {
            return valconv_float2float( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_SINT )
        {
            return valconv_sint2float( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_UINT )
        {
            return valconv_uint2float( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_BOOL )
        {
            return valconv_bool2float( to, tolen, from, fromlen );
        }
        else
        {
            return false;
        }
    }
    else if( totyp >= TSCH_SINT )
    {
        if( fromtyp >= TSCH_UTF8 )
        {
            return false;
        }
        else if( fromtyp >= TSCH_FLOAT )
        {
            return valconv_float2sint( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_SINT )
        {
            return valconv_sint2sint( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_UINT )
        {
            return valconv_uint2sint( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_BOOL )
        {
            return valconv_bool2sint( to, tolen, from, fromlen );
        }
        else
        {
            return false;
        }
    }
    else if( totyp >= TSCH_UINT )
    {
        if( fromtyp >= TSCH_UTF8 )
        {
            return false;
        }
        else if( fromtyp >= TSCH_FLOAT )
        {
            return valconv_float2uint( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_SINT )
        {
            return valconv_sint2uint( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_UINT )
        {
            return valconv_uint2uint( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_BOOL )
        {
            return valconv_bool2uint( to, tolen, from, fromlen );
        }
        else
        {
            return false;
        }
    }
    else if( totyp >= TSCH_BOOL )
    {
        if( fromtyp >= TSCH_UTF8 )
        {
            return false;
        }
        else if( fromtyp >= TSCH_FLOAT )
        {
            return valconv_float2bool( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_SINT )
        {
            return valconv_sint2bool( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_UINT )
        {
            return valconv_uint2bool( to, tolen, from, fromlen );
        }
        else if( fromtyp >= TSCH_BOOL )
        {
            return valconv_bool2bool( to, tolen, from, fromlen );
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

static tsch_size_t v_tausch_flater_rd_donotuse( tausch_flater_t *flat, tausch_ntype_t typ, uint8_t *buf,
    tsch_size_t len, va_list argptr )
{
    tsch_size_t rv = 0;
    tausch_flater_t fl = tausch_flater_clone( flat );

    // go to position if indexes are provided
    v_tausch_flater_go_to( &fl, argptr );

    if( fl.idx == 0 ) return 0;   // finding the item has failed, nothing to read

    tsch_size_t vlen = tausch_iter_vlen( &fl.iter );   // take the value length from iterator

    // now we are in position for reading
    if( ( (fl.row.ntype == TSCH_BLOB) || (fl.row.ntype == TSCH_UTF8)) && (typ == TSCH_BLOB) )
    {
        // using BLOB read method
        tausch_blob_t tmp = { .buf = buf, .len = len };
        rv = tausch_iter_read_blob( &fl.iter, &tmp );
    }
    else if( (fl.row.ntype >= TSCH_UINT) && (fl.row.ntype <= TSCH_FLOAT_64) )
    {
        // read out numbers and make the  type conversion if needed
        uint64_t res = 0;
        if( vlen > 8 ) rv = 0;
        else if( !tausch_iter_read_typX( &fl.iter, (uint8_t*)&res, vlen ) ) rv = 0;
        else if( valconv( typ, buf, len, fl.row.ntype, (uint8_t*)&res, vlen ) )
        {
            rv = len;
        }
    }
    else if( fl.row.ntype == TSCH_BOOL )
    {
        // read out TSCH_BOOL-s, make type conversion if needed
        uint64_t res = 0;
        if( vlen > 8 ) rv = 0;
        else if( !tausch_iter_read_bool( &fl.iter, (bool*)&res ) ) rv = 0;
        else if( valconv( typ, buf, len, fl.row.ntype, (uint8_t*)&res, (vlen > 0) ? vlen : 1 ) )
        {
            rv = len;
        }
    }
    else
    {
        rv = 0;   // we do not know how to read such thing
    }

    return rv;
}

tsch_size_t tausch_flater_rd_donotuse( tausch_flater_t *flat, tausch_ntype_t typ, uint8_t *buf, tsch_size_t len, ... )
{
    va_list argptr;

    if( flat->idx == 0 ) return 0;   // the flaterator is stuck

    if( flat->iter.buf == NULL ) return 0;   // no iterator provided

    va_start( argptr, len );

    return v_tausch_flater_rd_donotuse( flat, typ, buf, len, argptr );
}

tsch_size_t tausch_flater_rd_blob( tausch_flater_t *flat, tausch_blob_t *blob, ... )
{
    va_list argptr;

    if( flat->idx == 0 ) return 0;   // the flaterator is stuck

    if( flat->iter.buf == NULL ) return 0;   // no iterator provided

    va_start( argptr, blob );

    return v_tausch_flater_rd_donotuse( flat, TSCH_BLOB, blob->buf, blob->len, argptr );
}

tsch_size_t tausch_flater_write_any( tausch_flater_t *flat, tsch_size_t nam, tausch_ntype_t typ, uint8_t *buf,
    tsch_size_t len )
{
    if( flat->idx == 0 ) return 0;   // the flaterator is stuck

    if( flat->iter.buf == NULL ) return 0;   // no iterator provided

    tausch_flater_t fl = tausch_flater_clone( flat );

    // check if we already are at the nam
    if( fl.row.name != nam )
    {
        // we are not on the nam
        // go to position
        tausch_flater_go_to( &fl, nam );
        if( fl.idx == 0 )
        {
            // finding the item failed
            // go to stuffing then
            fl = tausch_flater_clone( flat );
            tausch_flater_go_to_stuffing( &fl );
            fl.iter.buf = NULL;
            tausch_flater_go_to( &fl, nam );
            fl.iter.buf = flat->iter.buf;
        }
    }

    if( flat->idx == 0 ) return 0;   // failed to find location for writing

    if( (!tausch_iter_is_stuffing( &fl.iter )) && (!tausch_iter_is_eof( &fl.iter )) )
    {
        if( (fl.row.ntype == TSCH_COLLECTION) || (fl.row.ntype == TSCH_VARIADIC) )
        {
            return 0;   // not right place to add scope
        }
    }

    //analyze the method we need to handle
    if( (typ == TSCH_BLOB) || (typ == TSCH_UTF8) )
    {
        if( (fl.row.ntype == TSCH_BLOB) )
        {
            tausch_blob_t tmp = { .buf = buf, .len = len };
            return tausch_iter_write_blob( &fl.iter, fl.row.item, &tmp );
        }
        else if( (fl.row.ntype == TSCH_UTF8) )
        {
            return tausch_iter_write_utf8( &fl.iter, fl.row.item, (char*)buf );
        }
        else
        {
            return 0;   // illegal write possibility
        }
    }
    else if( (typ >= TSCH_BOOL) && (typ <= TSCH_FLOAT_64) )
    {
        uint8_t result[8] = { 0 };
        tsch_size_t tolen = valuelengths[fl.row.ntype];

        if( (fl.iter.tag != 0) & (!tausch_iter_is_end(&fl.iter)) )
        {
            tolen = tausch_iter_vlen( &fl.iter );
        }
        else if( tolen > 8 )
        {
            tolen = 8;   // on writing clamp the value field to max
        }
        else
        {
            // length in place
        }

        if( tolen == 0 )
        {
            // here we are overwriting tag only boolean
            if( fl.row.ntype != TSCH_BOOL )
            {
                return 0;   // disablec combination
            }
            else
            {
                // go through conversion
                if( (buf != NULL) && (!valconv( TSCH_BOOL, result, 1, typ, buf, len )) ) return 0;   // no conversion
                else result[0] = (uint8_t)true;
                if( tausch_iter_write_bool( &fl.iter, fl.row.item, buf ? (bool*)result : (bool*)NULL ) ) return 1;
                else return 0;
            }
        }
        else if( (fl.row.ntype >= TSCH_BOOL) && (fl.row.ntype <= TSCH_FLOAT_64) )
        {
            if( (len == 0) )
            {
                return 0;   // illegal combination
            }
            else if( buf == NULL )
            {
                // writing cleared field
                return tausch_iter_write_typX( &fl.iter, fl.row.item, NULL, len );
            }
            else
            {
                // go through conversion
                if( !valconv( fl.row.ntype, result, tolen, typ, buf, len ) ) return 0;   // no conversion
                return tausch_iter_write_typX( &fl.iter, fl.row.item, result, tolen );
            }
        }
        else
        {
            return 0;   // not supported
        }
    }

    return 0;
}

tsch_size_t tausch_flater_write_blob( tausch_flater_t *flat, tsch_size_t nam, tausch_blob_t *blob )
{
    return tausch_flater_write_any( flat, nam, TSCH_BLOB, blob->buf, blob->len );
}

tsch_size_t tausch_flater_write_str( tausch_flater_t *flat, tsch_size_t nam, char *str )
{
    return tausch_flater_write_any( flat, nam, TSCH_UTF8, (uint8_t*)str, strnlen( str, flat->iter.ebuf ) );
}

bool tausch_flater_write_scope( tausch_flater_t *flat, tsch_size_t nam, tausch_flater_scope_writer_f writer )
{
    bool rv = true;

    tausch_flater_t fl_ini = tausch_flater_clone( flat );
    // go to stuffing or EOF, also advance the flaterator in its tree
    if( !tausch_flater_go_to_stuffing( &fl_ini ) ) return false;   // no space for writing

    if( tausch_iter_is_eof( &fl_ini.iter ) )
    {
        // extend the iter to the end of buffer?
        fl_ini.iter.next = fl_ini.iter.ebuf - 1;
        if( fl_ini.iter.next <= fl_ini.iter.idx ) return false;   // absolutely no space to write anything
        tausch_iter_format( &fl_ini.iter );
        tausch_iter_write_stuffing( &fl_ini.iter, fl_ini.iter.next - fl_ini.iter.idx );
    }

    fl_ini.iter.buf = NULL;
    tausch_flater_go_to( &fl_ini, nam );
    //fl_ini.iter.buf = flat->iter.buf;
    // verify that this is actually a scope tag
    if( fl_ini.row.sub == 0 ) return false;   // not a scope tag

    tausch_flater_t fl = tausch_flater_clone( &fl_ini );
    fl_ini.iter.buf = flat->iter.buf;
    *flat = fl_ini;

    // change the buffer space to hold also EOS
    fl.iter.buf = &flat->iter.buf[fl.iter.idx];
    fl.iter.next -= fl.iter.idx;
    fl.iter.idx = 0;
    fl.iter.ebuf = fl.iter.next - 1;
    fl.iter.next -= 2;
    if( fl.iter.next <= fl.iter.idx ) return false;   // absolutely no space to write anything
    fl.iter.val = TSCH_NOTHING;
    fl.iter.buf[fl.iter.next] = 7;   // write the EOF into place
    fl.iter.lc = 0;
    rv = rv && tausch_iter_write_stuffing( &fl.iter, fl.iter.next - fl.iter.idx );

    // perform the scope writing
    rv = rv && tausch_iter_write_scope( &fl.iter, fl.row.item );
    fl = tausch_flater_clone( &fl );   // just enter scope
    rv = rv && writer( &fl );   // call the scope contents writing function
    rv = rv && tausch_iter_go_to_stuffing( &fl.iter );   // advance the iterator to stuffing or fake T7
    if( rv ) fl.iter.ebuf ++;   // restore the buffer end
    rv = rv && tausch_iter_write_end( &fl.iter );   // exit the iterator from the scope

    // verify if the scope is collection and repeated tag is inserted into message
    if( rv && (fl_ini.row.ntype == TSCH_COLLECTION) )
    {
        tausch_iter_t a = fl_ini.iter;
        a.next = a.idx;
        a.val = a.idx;
        a.lc = 0;
        tausch_iter_next( &a );
        tausch_iter_enter_scope( &a );
        if( !tausch_iter_next( &a ) ) rv = false;
        tausch_iter_t b = a;
        // fixme, we are using the slowliest x^2 complexity search here
        while( rv && tausch_iter_next( &a ) )
        {
            if( b.tag == a.tag ) rv = false;   // this tag is used in more than one place
            if( tausch_iter_next( &b ) )
            {
                a = b;
            }
        }
    }

    if( !rv ) tausch_iter_erase( &fl_ini.iter );   // erase everithing that was written
    if( tausch_iter_is_complete( &flat->iter ) )
    {
        tausch_iter_t *iter = &flat->iter;
        iter->next = iter->idx;
        iter->val = iter->idx;
        iter->lc = 0;
        iter->vlen = 0;
        if( ! tausch_iter_next( iter ) )
        {
            // what ? failed ??
            iter->ebuf = 0;
            return 0;
        }
    }

    return rv;
}

