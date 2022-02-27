/*
 * tauschema_check.h
 *
 *  All rights reserved.
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

#ifndef SRC_TAUSCHEMA_CHECK_H_
#define SRC_TAUSCHEMA_CHECK_H_

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
} tausch_ntype_t;

/**
 * Structure of defining the flat-tree row
 */
typedef struct
{
    /// Binary coded item number
    uint16_t item;

    /// Name of the item (for human / json)
    char *name;

    /// Description of the item, help text
    char *desc;

    /// type number
    uint16_t ntype;

    /// table index to first subitem
    uint16_t sub;

    /// table index to next item on same scope
    uint16_t next;
} tausch_flatrow_t;


#endif /* SRC_TAUSCHEMA_CHECK_H_ */
