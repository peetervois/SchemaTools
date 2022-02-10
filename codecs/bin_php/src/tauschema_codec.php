<?php
/**
 * TauSchema Codec PHP
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
 */
namespace tauschema;

class Iterator
{

    /**
     * Initiate the iterator structure and use the message to parse.
     * The iterator works on pre-allocated binary memory blob - string in PHP.
     * The binary message can have at most of the number of bytes of the message.
     *
     * if $message is positive integer, then empty buffer will be allocated.
     * if $message is string, then the string is referenced.
     *
     * @param
     *            string | int $message - reference to buffer or integer
     */
    function __construct(&$message)
    {
        if (is_int($message) && ($message > 0)) {
            // start of empty buffer with length
            $msg = str_repeat(chr(0), $message);
            $msg[0] = chr(7);
            $this->p_msg = &$msg;
        } else if (is_string($message)) {
            // it is preallocated memory
            $this->p_msg = &$message;
        } else {
            // incorrect argument given
            $msg = "\0x07";
            $this->p_msg = &$msg;
        }
        $this->p_ebuf = strlen($this->p_msg);
    }

    /**
     * Return true if the character is EOF (End of File)
     *
     * @return boolean
     */
    function is_eof()
    {
        $v = ord($this->p_msg[$this->p_idx]);
        if ((($v & 3) == 3) && ($v > 3)) {
            return true;
        }
        return false;
    }
    
    /**
     * Return true if the characer is EOS (End of Scope) or EOF (End of File)
     * 
     * @return boolean
     */
    function is_end()
    {
        $v = ord($this->p_msg[$this->p_idx]);
        if (($v & 3) == 3) {
            return true;
        }
        return false;
    }

    /**
     * Format the end of the buffer.
     */
    function format()
    {
        $this->p_msg[$this->p_next] = chr(7);
    }

    /**
     * Return true if iterator is ok; false if it is broken
     *
     * @return boolean
     */
    function is_ok()
    {
        $notok = false;
        $notok = $notok || ($this->p_ebuf == - 1); // there is no buffer pointed
        $notok = $notok || ($this->p_idx == - 1); // idx is pointing to nothing
        $notok = $notok || ($this->p_idx >= $this->p_ebuf); // index is out of buffer or end of buffer
        $notok = $notok || ($this->p_next == - 1); // next is pointing to nothing
        $notok = $notok || ($this->p_next > $this->p_ebuf); // next is pointing out of buffer
        $notok = $notok || (($this->p_next == $this->p_idx) && ($this->p_val != $this->p_next)); // iter->p_val has incorrect value
        return ! $notok;
    }

    /**
     * Return true if the iterator is complete
     *
     * @return boolean
     */
    function is_complete()
    {
        // we do not check if iter is ok here, because that would reduce dimensions
        return $this->p_val != $this->p_next;
    }

    /**
     * Return true if the value is null.
     *
     * @return boolean
     */
    function is_null()
    {
        return $this->p_val == - 1;
    }

    /**
     * Return true if the iterator is clean for write.
     *
     * @return boolean
     */
    function is_clean()
    {
        if (! $this->is_ok())
            return false;
        return ($this->p_next == $this->p_idx) && ($this->p_val == $this->p_next);
    }

    /**
     * Return size of the stuffing when the iter is stuffing, otherwise 0
     *
     * @return number
     */
    function is_stuffing()
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if ($this->p_tag != 0)
            return 0;
        return $this->p_next - $this->p_idx;
    }
    
    /**
     * Return amount of free space in buffer
     * 
     * @throws \Exception
     * @return number
     */
    function buff_free()
    {
        if( $this->p_ebuf <= 0 )
            throw new \Exception("No buffer attached");
        $ti = $this->clone();
        do
        {
            $ti->decode_to_end();
        }
        while( ! $ti->is_eof() );
        return $this->p_ebuf - $ti->p_idx -1;
    }

    /**
     * Decode from binary buffer the variable length unsigned integer
     *
     * @return number
     */
    function decode_vluint()
    {
        $rv = 0;
        $x = 0;
        $s = 0;
        if ($this->p_next != $this->p_val)
            throw new \Exception("Incorrect moment to read vluint");
        do {
            if (! $this->is_ok()) {
                return -1;
            }
            if ($this->p_next >= $this->p_ebuf) {
                $this->p_ebuf = - 1; // ran out of buffer
                return - 1;
            }
            $x = ord($this->p_msg[$this->p_next]);
            if (($s + 1) < 32) {
                // we do not support bigger numbers for vluint
                $rv |= ($x & 0x7f) << $s;
                $s += 7;
            }
            $this->p_next += 1;
        } while ((($x & 0x80) == 0x80) && ($s < 32));
        $this->p_val = $this->p_next;
        return $rv;
    }

    /**
     * Encode to binary buffer the variable length unsigned integer.
     *
     * @param integer $val
     *            - the value to encode
     * @return boolean - true on success.
     */
    function encode_vluint($val)
    {
        $x = $val;
        $b = 0;
        if ($this->is_complete())
            return false; // the iterator is complete already
        do {
            if (! $this->is_ok())
                return false;
            if ($x & (~ 0x7f))
                $b = 0x80;
            else
                $b = 0x00;
            $b |= $x & 0x7f;
            $this->p_msg[$this->p_next] = chr($b);
            $this->p_next ++;
            $x >>= 7;
        } while ($x != 0);
        $this->p_val = $this->p_next;
        return true;
    }

    /**
     * Calculate memory length of the resulting vluint.
     *
     * @param number $val
     * @return number
     */
    function vluint_len($val)
    {
        $rv = 1;
        for ($i = 0x80; ($i <= $val) && ($i > 0); $i <<= 7) {
            $rv ++;
        }
        return $rv;
    }

    /**
     * Decodes from binary buffer next TLV item and stores info
     * inside iter element.
     * It does not skip over stuffing.
     * When returning false, then the iterator is no longer usable.
     *
     * @return boolean - true on success, false on failure.
     */
    function decode_next()
    {
        $tag = 0;
        $len = 0;
        $this->p_idx = $this->p_next;
        $this->p_val = $this->p_next;
        while (1) {
            $tag = $this->decode_vluint();
            if (! $this->is_ok())
                return false;
            if (($tag & 3) == 3) {
                // end of scope
                if (($this->p_scope == 0) || ($tag > 3)) {
                    // at the root level we stay at the END marker
                    // when tag part of the END is not 0, then it is also end of file
                    $this->p_next = $this->p_idx;
                    $this->p_val = $this->p_idx;
                    $this->p_tag = - 1;
                    $this->p_vlen = 0;
                    $this->p_scope = 0;
                    $this->p_lc = 0;
                    return false;
                }
                $this->p_scope -= 1;
                $this->p_idx = $this->p_next;
                continue;
            }
            break;
        }
        $len = 0;
        if (($tag & 1) == 1) {
            // collection or variadic tag
            $this->p_scope += 1;
        }
        if (($tag & 2) == 2) {
            $len = $this->decode_vluint();
            if (! $this->is_ok())
                return false;
        }
        $this->p_lc = $tag & 3;
        $this->p_tag = $tag >> 2;
        $this->p_vlen = $len;
        if ($len > 0) {
            if ((($this->p_next + $len) <= $this->p_next) || (($this->p_next + $len) > $this->p_ebuf)) {
                $this->p_val = - 1;
                $this->p_ebuf = - 1;
                return false;
            }
            $this->p_val = $this->p_next;
            $this->p_next += $len; // idx may now equal to exbuf
        } else {
            $this->p_val = - 1;
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
     * @return boolean
     */
    function write_next()
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        $this->p_idx = $this->p_next;
        $this->p_val = $this->p_idx;
        $this->p_vlen = 0;
        $this->p_tag = 0;
        $this->p_lc = 0;
        // note: iter->p_scope stays as it is
        return true;
    }

    /**
     * Advance the iterator to the element next item on the same scope.
     *
     * @return boolean - true on succes, false on failure.
     */
    function decode_to_next()
    {
        $scope = $this->p_scope;
        do {
            if (! $this->decode_next())
                return false;
            if (! $this->is_ok())
                return false;
        } while ($this->p_scope > $scope);
        return true;
    }

    /**
     * Advance the iterator to the element next item after end of current scope
     *
     * @return boolean
     */
    function decode_to_end()
    {
        $scope = $this->p_scope;
        do {
            if (! $this->decode_next()) {
                if (! $this->is_ok())
                    return false;
                else
                    return true; // EOF
            }
        } while ($this->p_scope >= $scope);
        return true;
    }

    /**
     * Advance the iterator to the next stuffing in the scope or next element
     * after end of scope or to the EOF
     *
     * @return boolean
     */
    function decode_to_stuffing()
    {
        $scope = $this->p_scope;
        do {
            if (! $this->decode_next()) {
                if (! $this->is_ok())
                    return false;
                else
                    return true; // EOF
            }
            if (($this->p_tag == 0) && ($this->p_lc != 1) && ($this->p_scope == $scope)) {
                // stuffing has been found
                return true;
            }
        } while ($this->p_scope >= $scope);
        return false;
    }

    /**
     * Advance the iterator to the next tag in the scope or next element
     * after end of scope
     *
     * @param number $tag
     * @return boolean
     */
    function decode_to_tag($tag)
    {
        $scope = $this->p_scope;
        do {
            if (! $this->decode_next()) {
                if (! $this->is_ok())
                    return false;
                else
                    return false; // EOF the tag was not found
            }
            if (! $this->is_ok())
                return false;
            if (($this->p_tag == $tag) && ($this->p_lc != 3) && (($this->p_scope - ($this->p_lc == 1)) == $scope)) {
                // stuffing has been found
                return true;
            }
        } while ($this->p_scope >= $scope);
        return false;
    }

    /**
     * Turn the element pointed by iterator into stuffing.
     * If the iterator is incomplete, then it creates new stuffing.
     *
     * @param int $len
     * @return boolean
     */
    function write_stuffing(int $len)
    {
        // FIXME: check if the usage of this method matches what it does, like what happens with idx
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        $totlen = $this->p_next - $this->p_idx;
        $cpy = $this->clone(); // copy the original iterator
        $is_eof = $this->is_eof();
        if (! $this->is_complete()) {
            // the iterator is incomplete
            $totlen = $len;
        }
        $finalize = function (bool $rv) use ($is_eof, $cpy) {
            $this->p_tag = 0;
            if (! $rv)
                $this->copy_from($cpy);
            if ($is_eof && ($this->p_next < $this->p_ebuf))
                $this->format();
            return $rv;
        };
        if ($totlen < 1)
            return $finalize(false);
        if ((($this->p_idx + $totlen) <= $this->p_idx) || (($this->p_idx + $totlen) > $this->p_ebuf))
            return $finalize(false);
        $this->p_next = $this->p_idx;
        $this->p_val = $this->p_next;
        $tlv = 0;
        if ($totlen == 1) {
            if (! $this->encode_vluint($tlv))
                return $finalize(false);
            $this->p_val = - 1;
            $this->p_vlen = 0;
        } else {
            $tlv = 2;
            if (! $this->encode_vluint($tlv))
                return $finalize(false);
            $tlv = $totlen - $this->p_next + $this->p_idx;
            $xv = $tlv;
            $lv = $this->vluint_len($xv);;
            // find the amount of exact data
            do {
                $xv = $tlv - $lv;
                $lv = $this->vluint_len($xv);
                
            } while (($xv + $lv) != $tlv);
            if (! $this->encode_vluint($xv))
                return $finalize(false);
            $this->p_val = $this->p_next;
            $this->p_vlen = $xv;
            for ($i = 0; $i < $xv; $i ++)
                $this->p_msg[$this->p_val + $i] = chr(0);
        }
        $this->p_next = $this->p_idx + $totlen;
        return $finalize(true);
    }

    /**
     * Erase the current item, turn fully into stuffing
     * 
     * @throws \Exception
     * @return unknown
     */
    function erase()
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if (! $this->is_complete())
            throw new \Exception("only complete iterator can erase element");
        if ( $this->is_end() )
            throw new \Exception("EOF or EOS can not be erased");
        return $this->write_stuffing( $this->p_next - $this->p_idx );
    }

    /**
     * Open new scope on the binary stream.
     * Scopes are COLLECTION or VARIADIC
     *
     * @param int $tag
     * @return boolean
     */
    function write_scope(int $tag)
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if ($this->is_complete())
            throw new \Exception("scope can only be appended");
        if ($this->p_scope == - 1)
            throw new \Exception("no more scopes can be added");
        $k = $tag;
        $k <<= 2;
        $k += 1;
        $is_eof = $this->is_eof();
        if (! $this->encode_vluint($k) || ($this->p_next >= $this->p_ebuf)) {
            $this->p_next = $this->p_idx;
            if ($is_eof && ($this->p_next < $this->p_ebuf))
                $this->format();
            return false;
        }
        if ($is_eof && ($this->p_next < $this->p_ebuf))
            $this->format();
        $this->p_val = - 1;
        $this->p_lc = 1;
        $this->p_scope += 1;
        $this->p_tag = $tag;
        return true;
    }

    /**
     * Close lastly open scope
     *
     * @return boolean
     */
    function write_end()
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if ($this->is_complete())
            throw new \Exception("End of scope can only be appended");
        if ($this->p_scope <= 0)
            throw new \Exception("Can not close root scope");
        $k = 3;
        $is_eof = $this->is_eof();
        if (! $this->encode_vluint($k) || ($this->p_next >= $this->p_ebuf)) {
            $this->p_next = $this->p_idx;
            if ($is_eof && ($this->p_next < $this->p_ebuf))
                $this->format();
            return false;
        }
        if ($is_eof && ($this->p_next < $this->p_ebuf))
            $this->format();
        $this->p_val = - 1;
        $this->p_lc = 3;
        $this->p_scope -= 1;
        $this->p_tag = - 1;
        return true;
    }

    /**
     * Read the iterator value field as BOOL
     *
     * @param
     *            reference boolean $value - resulting value
     * @return boolean - true if read was successful, false - if unsuccessful
     */
    function read_bool(bool &$value)
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if (! $this->is_complete())
            return false;
        if ($this->is_stuffing()) {
            $value = false;
            return true;
        }
        if ($this->p_vlen == 0) {
            $value = true;
            return true;
        } else
            for ($i = 0; $i < $this->p_vlen; $i ++) {
                if (ord($this->p_msg[$this->p_val + $i]) != 0) {
                    $value = true;
                    return true;
                }
            }
        $value = false;
        return true;
    }

    /**
     * Write the iterator value field as BOOL.
     * When the value is NULL then the value is considered true and it is stored
     * in the field in shortest possible way (tag only). Otherwise as uint8_t
     * or the entire existing field is filled with value.
     *
     * @param int $tag
     * @param boolean $value
     * @return boolean
     */
    function write_bool(int $tag, $value = NULL)
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        $val = 0;
        if (is_null($value)) {
            $val = 1;
        } else {
            if (! is_bool($value))
                throw new \Exception("Argument must be of type NULL or bool.");
            $val = $value ? 1 : 0;
        }
        $is_eof = $this->is_eof();
        $cpy = $this->clone();
        $finalize = function (bool $rv) use ($is_eof, $cpy) {
            if (! $rv)
                $this->copy_from($cpy);
            if ($is_eof && ($this->p_next < $this->p_ebuf))
                $this->format();
            return $rv;
        };

        if (! $this->is_complete()) {
            // Note if you want to append it in non space saving way, then
            // before calling this method, do tausch_write( iter, &tag, &value, 1 )
            if (is_null($value)) {
                // Append the element in a space saving way.
                $tlv = $tag << 2;
                if (! $this->encode_vluint($tlv))
                    return finalize(false);
                $this->p_val = - 1;
            } else {
                // Write the item as uint8_t
                if (! $this->write($tag, $val, "UINT-8"))
                    return $finalize(false);
            }
            $this->p_tag = $tag;
            return $finalize(true);
        } else {
            // the iterator already has been read, we are overwriting
            $totlen = $this->p_next - $this->p_idx;
            $tm = $this->clone();
            $tm->p_next = $tm->p_idx;
            $tm->p_val = $tm->p_next;
            $tm->p_ebuf = $this->p_next;
            $bytes = $this->vluint_len($tag << 2);
            if ($totlen < $bytes) {
                return $finalize(false); // can not encode, not enough space in it
            } else if ($totlen <= ($bytes + 1)) {
                // only true value can be encoded
                if ($val == true) {
                    $tlv = $tag << 2;
                    if (! $tm->encode_vluint($tlv + 2))
                        return $finalize(false);
                    // append the single byte, length 0
                    if ($totlen > $bytes) {
                        $tm->p_msg[$tm->p_next] = chr(0);
                        $tm->p_next ++;
                    }
                    $this->p_tag = $tag;
                } else // val == false
                {
                    if (! $tm->write_stuffing($totlen))
                        return $finalize(false);
                    $this->p_tag = 0;
                }
                return $finalize(true);
            } else {
                // full TLV has to be written
                $tlv = $tag << 2;
                $tlv += 2; // encode the len too
                if (! $tm->encode_vluint($tlv))
                    return $finalize(false);
                $tlv = $totlen - $bytes - 1; // now: tlv is length
                if (! $tm->encode_vluint($tlv))
                    return $finalize(false);
                while ($tm->p_next < $tm->p_ebuf) {
                    $tm->p_msg[$tm->p_next] = chr($val);
                    $tm->p_next ++;
                    $val = 0;
                }
                $this->p_tag = $tag;
                return $finalize(true);
            }
        }

        // return $finalize( true );
    }

    /**
     * Read the iterator value field as any variable
     *
     * format codes:
     * BOOL
     * SINT-8 SINT-16 SINT-32 SINT-64 SINT
     * UINT-8 UINT-16 UINT-32 UINT-64 UINT
     * FLOAT-32 FLOAT-64 FLOAT
     * UTF8 BLOB
     *
     * @param any $value
     *            - the reference to output variable (number or string)
     * @param string $format
     *            - the format code of the pack
     * @return number - the number of bytes read out
     */
    function read(&$value, string $format)
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        switch ($format) {
            // FIXME: in general we shall have machine dependent checkings
            case "BOOL":
                {
                    return $this->read_bool($value) ? 1 : 0;
                    break;
                }
            case "UINT-8":
                {
                    if ($this->p_vlen != 1)
                        return 0;
                    $res = unpack("C", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "SINT-8":
                {
                    if ($this->p_vlen != 1)
                        return 0;
                    $res = unpack("c", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "UINT-16":
                {
                    if ($this->p_vlen != 2)
                        return 0;
                    $res = unpack("v", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "SINT-16":
                {
                    if ($this->p_vlen != 2)
                        return 0;
                    $res = unpack("v", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    if ($value >= (1 << 15))
                        $value -= (1 << 16);
                    break;
                }
            case "UINT-32":
                {
                    if ($this->p_vlen != 4)
                        return 0;
                    $res = unpack("V", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "SINT-32":
                {
                    if ($this->p_vlen != 4)
                        return 0;
                    $res = unpack("V", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    if ($value >= (1 << 31))
                        $value -= (1 << 32);
                    break;
                }
            case "UINT-64":
                {
                    if ($this->p_vlen != 8)
                        return 0;
                    $res = unpack("P", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "SINT-64":
                {
                    if ($this->p_vlen != 8)
                        return 0;
                    $res = unpack("P", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    if ($value >= (1 << 63))
                        $value -= (1 << 64); // FIXME: it may have overflow of (1<<64)
                    break;
                }
            case "FLOAT-32":
                {
                    if ($this->p_vlen != 4)
                        return 0;
                    $res = unpack("g", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "FLOAT-64":
                {
                    if ($this->p_vlen != 8)
                        return 0;
                    $res = unpack("e", $this->p_msg, $this->p_val);
                    if ($res == false)
                        return 0;
                    $value = $res[1];
                    break;
                }
            case "SINT":
            case "UINT":
            case "FLOAT":
                {
                    if ($this->p_vlen <= 0)
                        return 0;
                    return $this->read($value, $format . "-" . $this->p_vlen);
                }
            case "UTF-8":
            case "BLOB":
                {
                    if ($this->p_vlen <= 0)
                        return 0;
                    $value = substr($this->p_msg, $this->p_val, $this->p_vlen);
                    break;
                }
            default:
                return 0;
        }
        return $this->p_vlen;
    }

    /**
     * Write to the location of iterator any finite value.
     * When the iter already
     * contains an element, it does verify it the tags match and the lengths match.
     * When value is NULL, and format is ZERO a null item will be written.
     * When value is NULL and type is numeric (e.g. SINT-8) then entire value field
     * is replaced with 0x00.
     * When value is integer and format is BLOB or UTF-8, then integer length of
     * the zeroed blob is written.
     *
     * format codes:
     * BOOL
     * SINT-8 SINT-16 SINT-32 SINT-64
     * UINT-8 UINT-16 UINT-32 UINT-64
     * FLOAT-32 FLOAT-64
     * UTF8 BLOB
     * ZERO - turning the existing value field to \x00-s
     * NULL - nullification/stuffing (removing) the length-value filed.
     *
     * @param int $tag
     *            - the tag of the data item
     * @param unknown $value
     *            - the value field
     * @param string $format
     *            - format identifier
     * @return number|unknown
     */
    function write(int $tag, $value = NULL, string $format = "NULL")
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if ($format == "BOOL") {
            return $this->write_bool($tag, $value);
        }
        $is_eof = $this->is_eof();
        $cpy = $this->clone();
        $finalize = function (int $rv) use ($is_eof, $cpy) {
            if (! $rv) {
                $this->copy_from($cpy);
            }
            if ($is_eof && ($this->p_next < $this->p_ebuf)) {
                $this->format();
            }
            return $rv;
        };
        $write_val = function ($value, string $format) {
            $str = '';
            switch ($format) {
                case "SINT-8":
                case "UINT-8":
                    if (is_null($value))
                        $str = str_repeat(chr(0), 1);
                    else
                        $str = pack("C", $value);
                    break;
                case "SINT-16":
                case "UINT-16":
                    if (is_null($value))
                        $str = str_repeat(chr(0), 2);
                    else
                        $str = pack("v", $value);
                    break;
                case "SINT-32":
                case "UINT-32":
                    if (is_null($value))
                        $str = str_repeat(chr(0), 4);
                    else
                        $str = pack("V", $value);
                    break;
                case "SINT-64":
                case "UINT-64":
                    if (is_null($value))
                        $str = str_repeat(chr(0), 8);
                    else
                        $str = pack("P", $value);
                    break;
                case "FLOAT-32":
                    if (is_null($value))
                        $str = str_repeat(chr(0), 4);
                    else
                        $str = pack("g", $value);
                    break;
                case "FLOAT-64":
                    if (is_null($value))
                        $str = str_repeat(chr(0), 8);
                    else
                        $str = pack("e", $value);
                    break;
                case "UTF-8":
                case "BLOB":
                    if (is_int($value))
                        $str = str_repeat(chr(0), $value);
                    else if (is_string($value))
                        $str = &$value;
                    else
                        return $str;
                default:
                    return $str;
            }
            return $str;
        };
        if ($this->is_complete()) {
            // we are overwriting the meomry
            if ($this->p_tag != $tag)
                return $finalize(0);
            if (! is_null($value)) {
                // simple overwrite of the value
                $str = $write_val($value, $format);
                $len = strlen($str);
                if (($format == "UTF-8")) {
                    if (($len < $this->p_vlen)) {
                        // fill reminder with 0
                        $str .= str_repeat(chr(0), $this->p_vlen - $len);
                    } else {
                        // truncate overflowing part, and remove partial multibyte character at the end.
                        $str = substr($str, 0, $this->p_vlen);
                        // FIXME: the following disables haveing multibyte character to be the last even if it is complete.
                        $v = 0;
                        for ($i = $this->p_vlen - 1; ($i >= 0) && (ord($str[$i]) > 127) && ($v < (128 + 64)); $i --) {
                            $v = ord($str[$i]);
                            $str[$i] = chr(0);
                        }
                    }
                    $len = $this->p_vlen;
                }
                if ($this->p_vlen != $len)
                    return $finalize(0); // the length must match
                for ($i = 0; $i < $len; $i ++)
                    $this->p_msg[$this->p_val + $i] = $str[$i];
                return $len;
            } else if ($format == "ZERO") {
                // we are zeroing the value field
                for ($i = 0; $i < $this->p_vlen; $i ++)
                    $this->p_msg[$this->p_val + $i] = chr(0);
                return $this->p_vlen;
            } else {
                // we are turning the item to null in place
                if ($this->p_vlen == 0)
                    return $finalize(1); // it is already null
                $tag <<= 2;
                // start new iterator for overwriting
                $tm = $this->clone();
                $tm->p_next = $tm->p_idx;
                $tm->p_val = $tm->p_next;
                $tm->p_ebuf = $this->p_next;
                $tm->p_lc = 0;
                $tm->p_vlen = 0;
                if (! $tm->encode_vluint($tag))
                    return $finalize(0);
                $this->p_next = $tm->p_next;
                $this->p_val = - 1;
                $this->p_vlen = 0;
                $this->p_lc = 0;
                // fill the remainder with stuffing
                $stlen = $tm->p_ebuf - $tm->p_next;
                $tm->p_idx = $tm->p_next;
                if (! $tm->write_stuffing($stlen))
                    return $finalize(0);
                return $finalize(1);
            }
        } else if (! $this->is_clean()) {
            return $finalize(0); // if the iter is not clean we can not write
        } else {
            // we are appending to the stream
            $this->p_tag = $tag;
            $tag <<= 2;
            $str = '';
            if (! is_null($value))
                $str = $write_val($value, $format);
            $len = strlen($str);
            if ($len > 0)
                $tag += 2;
            $this->p_lc = $tag & 3;
            // write the tag
            if (! $this->encode_vluint($tag))
                return $finalize(0);
            if ($len == 0) {
                // null value was requested
                $this->p_val = - 1;
                $this->p_vlen = 0;
                return $finalize(1);
            }
            // write the len
            if (! $this->encode_vluint($len))
                return $finalize(0);
            // write the value
            if ((($this->p_next + $len) <= $this->p_next) || (($this->p_next + $len) > $this->p_ebuf))
                return $finalize(0); // overflow
            if (! is_null($value)) {
                for ($i = 0; $i < $len; $i ++)
                    $this->p_msg[$this->p_val + $i] = $str[$i];
            } else {
                for ($i = 0; $i < $len; $i ++)
                    $this->p_msg[$this->p_val + $i] = chr(0);
            }
            $this->p_next += $len;
            $this->p_vlen = $len;
            return $finalize($len);
        }
    }

    function tag()
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        return $this->p_tag;
    }

    function vlen()
    {
        if (! $this->is_ok())
            throw new \Exception("The iterator is not ok");
        if ($this->p_val == - 1)
            return 0;
        if ($this->is_complete())
            return $this->p_vlen;
        return 0;
    }

    function &message()
    {
        return $this->p_msg;
    }

    function idx()
    {
        return $this->p_idx;
    }

    /* ========================================================================== */

    /**
     * Pointer to the buffer end, if p_ebuf == -1, then the iter is invalid.
     */
    private int $p_ebuf = - 1;

    /**
     * The message string to compose or to parse.
     */
    private string $p_msg = '';

    /**
     * Index to start of element of current iterator.
     */
    private int $p_idx = 0;

    /**
     * Index to position of next item.
     */
    private int $p_next = 0;

    /**
     * Index to the value field or -1,
     *
     * if val == next, then the iter is incomplete,
     * if val == next, then the iter is incomplete,
     * if val == next, then the iter is incomplete,
     * if val == -1, then the value is null.
     */
    private int $p_val = 0;

    /**
     * Tag value of the item.
     */
    private int $p_tag = - 1;

    /**
     * Length of the value part.
     */
    private int $p_vlen = 0;

    /**
     * The scope depth of the structure.
     */
    private int $p_scope = 0;

    /**
     * The l and c bits of the tag,
     *
     * if lc == 4 then
     * the iterator points to end of buffer.
     */
    private int $p_lc = 0;

    /**
     * Copy the values of other iterator to this.
     *
     * @param unknown $iter
     */
    function copy_from($iter)
    {
        $this->p_ebuf = $iter->p_ebuf;
        $this->p_msg = &$iter->p_msg;
        $this->p_idx = $iter->p_idx;
        $this->p_next = $iter->p_next;
        $this->p_val = $iter->p_val;
        $this->p_tag = $iter->p_tag;
        $this->p_vlen = $iter->p_vlen;
        $this->p_scope = $iter->p_scope;
        $this->p_lc = $iter->p_lc;
    }

    /**
     * Clone properly this object
     *
     * @return \tauschema\Iterator
     */
    function clone()
    {
        $tmpbuf = 2;
        $rv = new Iterator($tmpbuf);
        $rv->copy_from($this);
        return $rv;
    }
}

?>