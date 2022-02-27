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

#include "tauschema_codec.h"

/**
 * Enumeration of the TLV primitives
 */
typedef enum
{
    TSCH_NONE = 0,
    TSCH_BOOL = 1,
    TSCH_UINT,
    TSCH_UINT_8,
    TSCH_UINT_16,
    TSCH_UINT_32,
    TSCH_UINT_64,
    TSCH_SINT,
    TSCH_SINT_8,
    TSCH_SINT_16,
    TSCH_SINT_32,
    TSCH_SINT_64,
    TSCH_FLOAT,
    TSCH_FLOAT_32,
    TSCH_FLOAT_64,
    TSCH_UTF8,
    TSCH_BLOB,
    TSCH_COLLECTION,
    TSCH_VARIADIC
} tausch_ntype_t;

typedef struct tausch_schema_s tausch_schema_t;
typedef struct tausch_flatrow_s tausch_flatrow_t;
typedef struct tausch_flaterator_s tausch_flater_t;

/**
 * Structure that describes how to read out the schema from the memory.
 *
 * The Schema is provided with following densely coded TLV:
 *
 * -------------------------------
 * names : BLOB = 1 # zero ending UTF-8 strings one after another
 *
 * descriptions : BLOB = 2 # zero ending UTF-8 strings one after another
 *
 * rows : BLOB = 3 # continuous array of VLUINT( indexes ) as FLATROW, FLATROW, ...
 *  #
 *  # When names and descriptions are given then the blob contains rows with:
 *  # FLATROW := VLUINT(item), VLUINT(name), VLUINT(type), VLUINT(sub), VLUINT(next), VLUINT(desc)
 *  #
 *  # When descriptions are not given, then the blob contains rows with:
 *  # FLATROW := VLUINT(item), VLUINT(name), VLUINT(type), VLUINT(sub), VLUINT(next)
 *  #
 *  # The name and desc are byte index into the blobs of names and descriptions.
 *  # When names are not given, then the name is as continuous enumerator of names 1,2,3,...
 *
 * compress : BLOB = 4 # initial compression table of string characters.
 *  #
 *  # When the compress element is included in the schema flat tree TLV message, all the strings
 *  # names and descriptions must contain compressed strings.
 * -------------------------------
 *
 */
struct tausch_schema_s
{
    /// pointer to the memory that does conation VLUINT array of numbers
    tausch_blob_t rows;

    /// pointer to the memory where name strings start
    tausch_blob_t names;

    /// pointer to the memory where description strings start
    tausch_blob_t descriptions;

// pointer to the memory where string compression table resides
//tausch_blob_t compress;
};

/**
 * Initiate the schema structure.
 *
 * @attention The tlv shall not be freed from memory as long the schema is used.
 *
 * @param schema : tausch_schema_t* - structure to be initiated.
 * @param tlv : uint8_t* - tensely coded TLV of the schema data.
 * @param len : size_t - the amount of bytes in tlv buffer.
 * @return : bool - true on success false on failure.
 */
bool tausch_schema_init( tausch_schema_t *schema, const uint8_t *tlv, tsch_size_t len );

/**
 * Find the name index from the names array using binary searching algorithm of the strings.
 *
 * @param schema : tausch_schema_t* - the schema from which the name is looked for.
 * @param name_x : UTF-8* - the name of the item.
 * @return size_t - index of the name or 0 when not found.
 */
tsch_size_t tausch_schema_name_n( tausch_schema_t *schema, char *name_x );

/**
 * Copy a string starting from index name_n in the strings blob to memory field
 * pointed by name_s with name_n available free space. In any case the retrieved with 0 ending.
 *
 * The name_n shall point to the correct index, i.e. to the beginning of the string, but
 * if it is not then the remainder of the string will be copied and total remainder available
 * is placed in return value.
 *
 * @param strings : tausch_blob_t* - pointer to the blob containing array of strings.
 * @param name_x : char* - pointer to memory where to store the name string.
 * @param name_len : size_t - amount of memory available for the name string.
 * @param name_n : size_t - the index in name.
 * @return size_t - number of bytes in the string (can be more than name_len) or 0 on failure.
 *
 * @see TAUSCH_SCHEMA_STR_NAME
 * @see TAUSCH_SCHEMA_STR_DESC
 */
tsch_size_t tausch_schema_str( tausch_blob_t *strings, char *name_x, tsch_size_t name_len, tsch_size_t name_n );

/**
 * Copy a name string to memory array nam_x that does have name index nam_n fromt schema sch.
 *
 * @param sch : tausch_schema_t* - pointer to the schema.
 * @param nam_x : char[] - a memory defined as array.
 * @param nam_n : size_t - the index of the string in schema.
 *
 * @see TAUSCH_SCHEMA_STR_DESC
 * @see tausch_schema_str
 *
 * @example
 *
 * do
 * {
 *      char name[40];
 *      size_t nlen = TAUSCH_SCHEMA_STR_NAME( &sc, name, idx );
 *      if( nlen == 0 )
 *      {
 *          // handle error of index not found
 *      }
 *      else if( nlen >= sizeof(name) )
 *      {
 *          // handle the case when the name is longer than we retrieved.
 *          // note last byte in name is 0.
 *      }
 *      else
 *      {
 *          // we retrieved all bytes of the name
 *      }
 * }
 * while( 0 );
 */
#define TAUSCH_SCHEMA_STR_NAME( sch, nam_x, nam_n ) \
		tausch_schema_str( &((sch)->names), (nam_x), sizeof(nam_x), (nam_n) )

/**
 * Copy a description string to memory array nam_x that does have name index nam_n fromt schema sch.
 *
 * @param sch : tausch_schema_t* - pointer to the schema.
 * @param nam_x : char[] - a memory defined as array.
 * @param nam_n : size_t - the index of the string in schema.
 *
 * @see TAUSCH_SCHEMA_STR_NAME
 * @see tausch_schema_str
 */
#define TAUSCH_SCHEMA_STR_DESC( sch, dsc_x, dsc_n ) \
        tausch_schema_str( &((sch)->descriptions), (dsc_x), sizeof(dsc_x), (dsc_n) )

/**
 * Structure of defining the flat-tree row
 */
struct tausch_flatrow_s
{
    /// Reference to the schema structure
    tausch_schema_t *schema;

    /// Binary coded item number, tag value in TLV
    tsch_size_t item;

    /// Index of the name of the item (for human / json) in tausch_schema_t::names
    tsch_size_t name;

    /// Index of the description of the item, help text in tausch_schema_t::descriptions
    tsch_size_t desc;

    /// primitive type number
    tausch_ntype_t ntype;

    /// table index to first subitem
    tsch_size_t sub;

    /// table index to next item on same scope
    tsch_size_t next;
};

/**
 * Initiate the flatrow structure.
 *
 * @param row : tausch_flatrow_t* - structure to initiate.
 * @param schema : tausch_schema_t* - the schema structure the row belongs to.
 * @return bool - true on success false on failure.
 */
bool tausch_flatrow_init( tausch_flatrow_t *row, tausch_schema_t *schema );

/** Function that does decode the flatrow from the table.
 *
 * @param row : tausch_flatrow_t - structure to fill with the data.
 * @param idx : size_t - index into rows memory.
 * @return bool - true on success, flase on failure.
 */
bool tausch_flatrow_decode( tausch_flatrow_t *row, tsch_size_t idx );

/**
 * Iterator into the flat three
 */
struct tausch_flaterator_s
{
    tausch_flatrow_t row;

    tsch_size_t scope;

    tsch_size_t idx;

    tausch_iter_t iter;

};

/**
 * Initiate the flaterator structure
 *
 * @param flat : tausch_flater_t* - the flaterator.
 * @param schema : tausch_schema_t* - the schema for verifications and automation.
 * @param msg : uint8_t* - the message buffer.
 * @param msg_len : size_t - the message buffer size.
 * @return bool - true on success false on failure.
 */
bool tausch_flater_init( tausch_flater_t *flat, tausch_schema_t *schema, uint8_t *msg, tsch_size_t msg_len );

/**
 * Reset the flaterator to beginning
 *
 * @param flat : tausch_flater_t* - pointer to the object to reset.
 * @return tausch_flater_t* - pointer to the same object as the argument.
 */
tausch_flater_t* tausch_flater_reset( tausch_flater_t *flat );

/**
 * Iterate in flat tree to the name in the sub-scope after the current position.
 * The first sub-scope is the root scope. In case it stops on collection
 * or variadic, it will enter into that scope on the beginning of the next
 * call of go_to.
 *
 * The index arguments list is relation from current position. Termination 0 will be
 * added by the macro.
 *
 * @param flat : tausch_flater_t* - the flaterator
 * @param ... : size_t - 0 terminated list of indexes of tlv item's name field
 * @return tausch_flater_t* - pointer to the same object as the argument
 *
 * @example
 *
 * (void)tausch_flater_go_to( &fl,
 *      TAUSCH_NAM_DEVICE_INFO_info,
 *      TAUSCH_NAM_DEVICE_INFO_name,
 *      TAUSCH_NAM_DEVICE_INFO_orig );
 *
 */
#define tausch_flater_go_to( flat, ... ) ({                                             \
    tausch_flater_t* tausch_flater_go_to_donotuse( tausch_flater_t*, ... );             \
    tausch_flater_go_to_donotuse( (flat), ##__VA_ARGS__, 0 ); })

/**
 * Advance the iterator to next element on message. Stays to the same scope
 * and jubps over subscopes.
 *
 * @param flat : tausch_flater_t* -the flaterator to advance.
 * @return tausch_flater_t* - the same object as argument.
 */
tausch_flater_t* tausch_flater_next( tausch_flater_t *flat );

/**
 * Advance the iterator to the EOF.
 *
 * @param flat : tausch_flater_t* -the flaterator to advance.
 * @return tausch_flater_t* - the same object as argument.
 */
tausch_flater_t* tausch_flater_go_eof( tausch_flater_t *flat );

/**
 * Get the current decoded flat tree row.
 *
 * @param flat : tausch_flater_t* -the flaterator.
 * @return tausch_flatrow_t*
 */
#define tausch_flater_get( flat ) (&(flat)->row)

/**
 * Clone the Flaterator. If the index is on scope, then change the current
 * scope to the indexed item on the cloned flaterator.
 *
 * @param flat : tausch_flater_t* -the flaterator to advance
 * @return tausch_flater_t* - the same object as argument
 *
 * @note If you do not want the scope change then use simple copy:
 *
 * @example
 *   // clone with scope change
 *   void a_function( tausch_flater_t *orig)
 *   {
 *      tausch_flater_t temp = tausch_flater_clone( orig );
 *   }
 *
 *   // clone without scope change
 *   void a_function( tausch_flater_t *orig)
 *   {
 *      tausch_flater_t temp = *orig ;
 *   }
 */
tausch_flater_t tausch_flater_clone( tausch_flater_t *flat );

/**
 * Get the tag of current item in string format. Will return NULL if there is
 * no string possible.
 *
 * @param flat : tausch_flater_t* - the flaterator object.
 * @return char* - pointer to the tag name string (utf-8).
 */
char* tausch_flater_tag_x( tausch_flater_t *flat );

/**
 * Get the tag of current item in string enumerator format.
 *
 * @param flat : tausch_flater_t* -the flaterator.
 * @return size_t - the enumerator of name.
 */
#define tausch_flater_tag_n( flat ) ((flat)->row.name)

/**
 * Check if the parameter matches with tag or name of current item in schema.
 *
 * @param flat : tausch_flater_t* -the flaterator.
 * @return bool - true if the item is currently pointed.
 */
#define tausch_flater_is_tag_n( flat, itm ) (tausch_flater_tag_n(flat) == (itm))

/**
 * Read the value based of current item. It does not change current flaterator
 * and does create a copy before it does advance to the id field.
 *
 * It does know if BLOB or UTF-8 is read and will call relevant methods.
 *
 * The terminating 0 will be attended by the macros to the flater_rd function call.
 *
 * @param flat : tausch_flater_t* - the flaterator to use basis for the reading.
 * @param typ : tausch_ntype_t - which primitive is pointed by the buffer.
 * @param buf : uint8_t* - pointer to the variable memory.
 * @param len : size_t - length of the buf in bytes.
 * @param ... : size_t - the 0 terminated list of indexes of tlv item's name field for location change before write.
 * @return size_t - number of data bytes copied, 0 on error.
 *
 */
#define tausch_flater_read( flat, value, ... ) ({                                                          \
    tsch_size_t tausch_flater_rd_donotuse( tausch_flater_t *, tausch_ntype_t , uint8_t *, size_t , ... );  \
    tsch_size_t tausch_flater_rd_blob( tausch_flater_t *, tausch_blob_t*, ... );  \
    tsch_size_t rv = tausch_flater_read_select( (flat), (value), ##__VA_ARGS__ ); rv; })

/**
 * Convenient macro for reading value from message into any kind of data containers.
 *
 * @param flat : tausch_falter_t* - pointer to the flaterotator
 * @param value : various* - pointer to the various data containers
 * @param ... : size_t - indexes of the names, does not need to be 0 ending
 * @return size_t - number of bytes read out, 0 on error.
 *
 * @example
 * do{
 *      uint32_t retv;
 *      if( tausch_flater_read( &fl, &retv,
 */
#define tausch_flater_read_select( flat, value, ... ) _Generic((value),                                                     \
 bool*          : tausch_flater_rd_donotuse((flat), TSCH_BOOL,     (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 uint8_t*       : tausch_flater_rd_donotuse((flat), TSCH_UINT_8,   (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 uint16_t*      : tausch_flater_rd_donotuse((flat), TSCH_UINT_16,  (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 uint32_t*      : tausch_flater_rd_donotuse((flat), TSCH_UINT_32,  (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 uint64_t*      : tausch_flater_rd_donotuse((flat), TSCH_UINT_64,  (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 int8_t*        : tausch_flater_rd_donotuse((flat), TSCH_SINT_8,   (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 int16_t*       : tausch_flater_rd_donotuse((flat), TSCH_SINT_16,  (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 int32_t*       : tausch_flater_rd_donotuse((flat), TSCH_SINT_32,  (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 int64_t*       : tausch_flater_rd_donotuse((flat), TSCH_SINT_64,  (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 float*         : tausch_flater_rd_donotuse((flat), TSCH_FLOAT_32, (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 double*        : tausch_flater_rd_donotuse((flat), TSCH_FLOAT_64, (uint8_t*)(value), sizeof((value)[0]), ##__VA_ARGS__,0), \
 tausch_blob_t* : tausch_flater_rd_blob((flat), (tausch_blob_t*)(value), ##__VA_ARGS__,0)  \
)

/**
 * Write the value based of current item into message. It does not change current flaterator
 * and does create a copy of it before the flaterator will be advanced to the field.
 *
 * It does know if BLOB or UTF-8 is written and will call relevant methods.
 *
 * It will try to advance into the position specified by name index at or after the current location.
 * It will enter into scope, when the iterator is at scope before function call.
 * If the item, a stuffing or EOF is not found it will fail with writing.
 *
 * @param flat : tausch_flater_t* - the flaterator to use basis for the writing.
 * @param name : size_t - index of tlv item's name field for location change before write.
 * @param value : mixed* - pointer to the variable memory.
 * @return size_t - number of data bytes copied, 0 on error.
 */
#define tausch_flater_write( flat, name, value ) ({                                                               \
  tsch_size_t tausch_flater_write_any( tausch_flater_t*, tsch_size_t, tausch_ntype_t, uint8_t*, tsch_size_t );    \
  tsch_size_t tausch_flater_write_blob( tausch_flater_t*, tsch_size_t, tausch_blob_t* );                          \
  tsch_size_t tausch_flater_write_str( tausch_flater_t*, tsch_size_t, char* );                                    \
  tsch_size_t rv = tausch_flater_write_select( (flat), (name), (value) ); rv; })

#define tausch_flater_write_select( flat, nam, value ) _Generic((value),                                         \
 bool*          : tausch_flater_write_any((flat), (nam), TSCH_BOOL,     (uint8_t*)(value), sizeof((value)[0]) ), \
 uint8_t*       : tausch_flater_write_any((flat), (nam), TSCH_UINT_8,   (uint8_t*)(value), sizeof((value)[0]) ), \
 uint16_t*      : tausch_flater_write_any((flat), (nam), TSCH_UINT_16,  (uint8_t*)(value), sizeof((value)[0]) ), \
 uint32_t*      : tausch_flater_write_any((flat), (nam), TSCH_UINT_32,  (uint8_t*)(value), sizeof((value)[0]) ), \
 uint64_t*      : tausch_flater_write_any((flat), (nam), TSCH_UINT_64,  (uint8_t*)(value), sizeof((value)[0]) ), \
 int8_t*        : tausch_flater_write_any((flat), (nam), TSCH_SINT_8,   (uint8_t*)(value), sizeof((value)[0]) ), \
 int16_t*       : tausch_flater_write_any((flat), (nam), TSCH_SINT_16,  (uint8_t*)(value), sizeof((value)[0]) ), \
 int32_t*       : tausch_flater_write_any((flat), (nam), TSCH_SINT_32,  (uint8_t*)(value), sizeof((value)[0]) ), \
 int64_t*       : tausch_flater_write_any((flat), (nam), TSCH_SINT_64,  (uint8_t*)(value), sizeof((value)[0]) ), \
 float*         : tausch_flater_write_any((flat), (nam), TSCH_FLOAT_32, (uint8_t*)(value), sizeof((value)[0]) ), \
 double*        : tausch_flater_write_any((flat), (nam), TSCH_FLOAT_64, (uint8_t*)(value), sizeof((value)[0]) ), \
 tausch_blob_t* : tausch_flater_write_blob((flat), (nam), (tausch_blob_t*)(value) ),  \
 char*          : tausch_flater_write_str((flat), (nam), (char*)(value) )  \
)


/**
 * Callback function type for writing scope contents. The scope is already opened and when the function
 * returns true, the scope will be closed.
 *
 * @param flat : tausch_flater_t* - the flaterator that is aligned to the beginning of the scope.
 * @return bool - true if all writing went well, false when the writing of scope shall be rolled back.
 */
typedef bool (*tausch_flater_scope_writer_f)( tausch_flater_t *flat );

/**
 * Write the scope (COLLECTION or VARIADIC) that has name index nam according to flat tree.
 * Writing of the scope body is handled by the writer callback function. Once the writer does return true
 * scope will be closed, otherwise the newly written items will be erased.
 *
 * When the COLLECTION is written, then at the end it will verify that all the element names are unique.
 * In case of duplicate element is written, the written scope will be erased and false will be returned.
 *
 * @param flat : tausch_flater_t* - the flaterator into position where to write.
 * @param nam : size_t - the name of the item in flat tree.
 * @param writer : tausch_flater_scope_writer_f - callback that shall write the body into the message.
 * @return bool - true when the scope writing was successful.
 *
 * @example with standard gnu11
 *
 * do
 * {
 *      rv = rv && tausch_flater_write_scope( &fl, TAUSCH_NAM_DEVICE_INFO_name, ({ bool _fn_( tausch_flater_t *sfl )
 *      {
 *          bool srv = true;
 *          uint32_t origin = get_my_missing_origin();
 *          srv = srv && ( tausch_flater_write_any( sfl, &origin, sizeof(origin), TAUSCH_NAM_DEVICE_INFO_orig ) > 0 );
 *          srv = srv && ( tausch_flater_write_any( sfl, NULL, 20, TAUSCH_NAM_DEVICE_INFO_data ) > 0 );
 *          return srv;
 *      };_fn_;}));
 * }
 * while(0);
 *
 * // or following style
 *
 * do
 * {
 *      bool nestfn_look_for_name( tausch_flater_t *sfl )
 *      {
 *          bool srv = true;
 *          uint32_t origin = get_my_missing_origin();
 *          srv = srv && ( tausch_flater_write_any( sfl, &origin, sizeof(origin), TAUSCH_NAM_DEVICE_INFO_orig ) > 0 );
 *          srv = srv && ( tausch_flater_write_any( sfl, NULL, 20, TAUSCH_NAM_DEVICE_INFO_data ) > 0 );
 *          return srv;
 *      }
 *      rv = rv && tausch_flater_write_scope( &fl, TAUSCH_NAM_DEVICE_INFO_name, nestfn_look_for_name );
 * }
 * while(0);
 *
 * // or following style
 *
 * do
 * {
 *      auto bool nestfn_look_for_name( tausch_flater_t *sfl );
 *      rv = rv && tausch_flater_write_scope( &fl, TAUSCH_NAM_DEVICE_INFO_name, nestfn_look_for_name );
 *      bool nestfn_look_for_name( tausch_flater_t *sfl )
 *      {
 *          bool srv = true;
 *          uint32_t origin = get_my_missing_origin();
 *          srv = srv && ( tausch_flater_write_any( sfl, &origin, sizeof(origin), TAUSCH_NAM_DEVICE_INFO_orig ) > 0 );
 *          srv = srv && ( tausch_flater_write_any( sfl, NULL, 20, TAUSCH_NAM_DEVICE_INFO_data ) > 0 );
 *          return srv;
 *      }
 * }
 * while(0);
 *
 *
 */
bool tausch_flater_write_scope( tausch_flater_t *flat, tsch_size_t nam, tausch_flater_scope_writer_f writer );

/*
 * Helper macros for combining indexes
 */
#ifndef COMB
#define COM1(X, Y) X##Y // helper macro
#define COMB(X, Y) COM1(X, Y)
#endif

/**
 * Macro sugar that helps to write the code more understandable of what it does. Requires gnu11 standard.
 *
 * @example
 *
 * rv = rv & TAUSCH_FLATER_WRITE_SCOPE( &fl, TAUSCH_NAM_DEVICE_INFO_name )
 * {
 *      // this is nested function scope that is placed as argument for write_scope
 *      bool srv = true;
 *      uint32_t origin = get_my_missing_origin();
 *      srv = srv && ( tausch_flater_write_any( sfl, &origin, sizeof(origin), TAUSCH_NAM_DEVICE_INFO_orig ) > 0 );
 *      srv = srv && ( tausch_flater_write_any( sfl, NULL, 20, TAUSCH_NAM_DEVICE_INFO_data ) > 0 );
 *      return srv;
 * }
 * TAUSCH_FLATER_CLOSE_SCOPE;
 *
 */
#define TAUSCH_FLATER_WRITE_SCOPE( fl, nam )                                                \
    ({                                                                                      \
    auto bool COMB( nestfn, __LINE__ )( tausch_flater_t* );                                 \
    bool rv = tausch_flater_write_scope( (fl), (nam), COMB( nestfn, __LINE__ ) );           \
    bool COMB( nestfn, __LINE__ )( tausch_flater_t* sfl )

#define TAUSCH_FLATER_CLOSE_SCOPE rv;});

#endif /* SRC_TAUSCHEMA_CHECK_H_ */
