<?php
/**
 * TauSchema Check PHP
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

include_once 'tauschema_codec.php';

class Flaterator
{

    /**
     * Initiate the flat tree iterator and message iterator.
     *
     * @param
     *            string | int $message - reference to buffer or integer
     * @param array $schema
     */
    function __construct(Array &$schema, &$message = Null)
    {
        $this->p_schema = $schema;
        $this->p_iter = new Iterator($message);
        $this->reset();
    }

    /**
     * Reset the flaterator to beginning
     *
     * @return \tauschema\Flaterator
     */
    function reset()
    {
        $this->p_idx = - 1;
        $this->p_scope = 0;
        $this->p_iter->reset();
        return $this;
    }

    /**
     * Iterate in flat tree to the ID in the subscope.
     * The first subscope is
     * the root scope. In case it stops on collection or variadic, it will
     * enter into that scope on the beginning of the next call of go_to.
     *
     * The argument can be relational from current position:
     * "name1.name2.name3"
     * or
     * [ 1, "name2", "name3 ]
     * or just a integer
     * 1
     * or just a string
     * "name1"
     *
     * if id is integer then it is searched as tag item
     *
     * if id is string then it is searched as item name
     *
     * if id is array then elements are treated as path to the item.
     *
     * @param
     *            int | string | array $id
     * @return \tauschema\Flaterator - this object
     */
    function go_to($id)
    {
        if ($this->p_idx == 0) {
            // the iterator is stuck
            return $this;
        }

        if (is_string($id)) {
            // check if the string is relational
            // like name1.name2.name3
            $arr = explode(".", $id);
            if (count($arr) > 1) {
                return $this->go_to($arr);
            }
        } else if (is_array($id)) {
            // the argument is relational array
            // like [ 1, "name2", "name3" ]
            for ($i = 0; ($i < count($id)) && ($this->p_idx != 0); $i ++) {
                $this->go_to($id[$i]);
            }
            return $this;
        }
        // from this on, the id is either simple string or number

        if (($this->p_idx > 0) && ($this->p_schema[$this->p_idx]["sub"] > 0) && ($this->p_scope != $this->p_idx)) {
            // the iterator has been stopping on the scope
            // in this case we advance the scope pointer
            $this->p_scope = $this->p_idx;
            // $this->p_idx = $this->p_schema[$this->p_idx]["sub"];
            if ((! is_null($this->p_iter)) && (! $this->p_iter->enter_scope())) {
                $this->p_idx = 0; // dead end
                return $this;
            }
        }

        // start from the beginning of the scope
        $this->p_idx = $this->p_schema[$this->p_scope]["sub"];

        $comp = function ($itm) {
            if (is_int($itm)) {
                return ($this->p_schema[$this->p_idx]["item"] == $itm);
            } else if (is_string($itm)) {
                return ($this->p_schema[$this->p_idx]["name"] == $itm);
            }
            return false;
        };

        while ($this->p_idx > 0) {
            if ($comp($id)) {
                // the item was found from schema, find it from the binary too
                if ((is_null($this->p_iter)) || $this->p_iter->go_to_tag($this->p_schema[$this->p_idx]["item"])) {
                    break;
                }
                $this->p_idx = 0;
            } else {
                $this->p_idx = $this->p_schema[$this->p_idx]["next"];
            }
        }

        return $this;
    }

    /**
     * Advance the iterator to next element on message.
     *
     * @return \tauschema\Flaterator
     */
    function next()
    {
        if (is_null($this->p_iter)) {
            // we do not have binary iterator, only flat tree iteration
            if ($this->p_idx == - 1) {
                // we have initiated iterator
                $this->p_idx = $this->p_schema[$this->p_scope]["sub"];
            }
            if ($this->p_idx > 0) {
                $this->p_idx = $this->get()["next"];
            }
        } else {
            // now if we iterate with binary message too, the approach is different
            if (! $this->p_iter->next()) {
                // iteration in binary to next failed
                $this->p_idx = 0;
                return $this;
            }

            // start search from the beginning of the scope of schema
            $this->p_idx = $this->p_schema[$this->p_scope]["sub"];

            while (($this->p_idx > 0) && ($this->p_iter->tag() != $this->get()["item"])) {
                $this->p_idx = $this->get()["next"];
            }
            // if the schemaitem was not found in the scope, then pidx is 0
        }
        return $this;
    }

    /**
     * Advance the iterator to the EOF
     *
     * @return \tauschema\Flaterator
     */
    function go_eof()
    {
        $this->reset();
        if (! is_null($this->p_iter)) {
            $this->p_iter->exit_scope();
            if (! $this->p_iter->is_eof()) {
                $this->reset();
                throw new \Exception("Advancing to EOF failed.");
            }
        }
        return $this;
    }

    /**
     * Get the flat tree row.
     * The is associative array with elements
     * "item"=>tlvid, "name"=>"jsonname", "type"=>"TYPENAME",
     * "desc"=>"long description"
     *
     * @return Array
     */
    function get()
    {
        return $this->p_schema[$this->p_idx];
    }

    /**
     * Clone the Flaterator.
     * If the index is on scope, then change the current
     * scope to the indexed item on the cloned flaterator
     *
     * @param boolean $enter
     *            - false if do not enter into scope
     * @return \tauschema\Flaterator
     */
    function clone($enter = true)
    {
        $rv = new Flaterator($this->p_schema);
        $rv->p_idx = $this->p_idx;
        $rv->p_scope = $this->p_scope;
        $rv->p_iter = is_null($this->p_iter) ? Null : $this->p_iter->clone();
        if ($enter && ($rv->p_idx > 0) && ($rv->p_schema[$rv->p_idx]["sub"] > 0) && ($this->p_scope != $this->p_idx)) {
            // the iterator has been stopping on the scope
            // in this case we advance the scope pointer
            $rv->p_scope = $rv->p_idx;
            // $this->p_idx = $this->p_schema[$this->p_idx]["sub"];
            if ((! is_null($this->p_iter)) && (! $rv->p_iter->enter_scope())) {
                $rv->p_idx = 0; // dead end
            }
        }
        return $rv;
    }

    /**
     * Get the tag of current item in string format
     *
     * @return string
     */
    function tag_s()
    {
        if ($this->p_idx > 0) {
            return $this->p_schema[$this->p_idx]["name"];
        }
        return "";
    }
    
    /**
     * Check if the parameter matches with tag or name of current item in schema
     * 
     * @param unknown $itm
     * @return boolean
     */
    function is_tag( $itm )
    {
        if (is_int($itm)) {
            return ($this->p_schema[$this->p_idx]["item"] == $itm);
        } else if (is_string($itm)) {
            return ($this->p_schema[$this->p_idx]["name"] == $itm);
        }
        return false;
    }

    /**
     * Read the value of current item.
     *
     * Note that the scope items will return true and end of scope false.
     * Blob and utf8 will return PHP strings.
     * It does return Null when the iterator is not correctly set.
     *
     * @param mixed $id
     * @return mixed value of the item.
     */
    function read($id = Null)
    {
        $rv = 0;

        if (is_null($this->p_iter))
            throw new \Exception("Can not call read without binary iterator");

        $fl = $this->clone();

        if (! is_null($id)) {
            // the index has been provided, use goto
            $fl->go_to($id);
        }

        if ($fl->p_idx == 0) {
            // finding the item has failed, nothing to read
            return Null;
        }

        // now we are in position
        if ($fl->p_iter->read($rv, $fl->get()["type"]) <= 0) {
            return Null;
        }

        return $rv;
    }

    /**
     * Write value into current item or add into the end of the message.
     * 
     * In case of mismatch with the schema, it will trhow exception.
     *
     * @param $value -
     *            the value or array to write
     * @param $id -
     *            the item name for writing
     * @return boolean true if the writing was successful
     */
    function write($value, $id = Null)
    {
        if (is_null($this->p_iter)) {
            throw new \Exception("Can not call write without binary iterator");
        }
        $rv = true;
        $fl = $this->clone();

        // first try to understand if we are overwriting or adding
        $over = ! $fl->p_iter->is_eof();

        if (($over) && (! is_null($id))) {
            // index has been provided, try to go to the position
            $fl->go_to($id);
        }
        // check if we have a valid index in flat tree
        $over = $over && ($fl->p_idx > 0);
        // we do not support overwriting of entire structures
        $over = $over && ($fl->get()["sub"] == 0);
        // and if value is array then it is request towrite structure
        $over = $over && (! is_array($value));

        if ((! $over) && is_string($id)) {
            // check if the string is relational
            // like name1.name2.name3
            $id = explode(".", $id); // even if only one element is added
        }

        if ((! $over) && is_array($id)) {
            // the argument is relational array
            // like [ "name1", "name2", "name3" ]
            for ($i = count($id) - 1; $i >= 0; $i --) {
                $value = Array(
                    "{$id[$i]}" => $value
                );
            }
        }

        if ($over) {
            // The situation is overwriting situation
            // write value in place with type verifications
            $rv = $rv && $fl->p_iter->write( $fl->get()["item"], $value, $fl->get()["type"] );
            // done
        } else if (! $over) {
            // Only successful option is to add to the end of message
            // The value is turned into array in any case
            $fl->go_eof();
            // produce only flat tree iterator
            $it = $fl->clone();
            $it->p_iter = Null;
            // recursion write primitives from array, returns false on failure
            // throws exception on misuse or incorrect data structure
            $primitives = function ($a, $fl, $it) use (&$primitives) {
                $rv = true;
                if (is_string($a) || is_integer($a)) {
                    // a tagonly boolean = True is requested to be written
                    $mit = $it->clone()->go_to($a);
                    if (! $mit->is_tag($a)) {
                        throw new \Exception(" the key {$a} is not in schema");
                    }
                    if ($mit->get()["type"] != "BOOL") {
                        throw new \Exception(" the key {$a} is not BOOL in schema");
                    }
                    $rv = $rv && ! $fl->p_iter->next();
                    $rv = $rv && $fl->p_iter->write($mit->get()["item"], Null, "BOOL");
                    $rv = $rv && ! $fl->p_iter->next();
                    // done
                } else if (is_array($a)) {
                    if (! (array_values($a)!==$a)) {
                        // it is a case with variadic, we need list for its values
                        // plain list can be removed, we simply recurse through it
                        for ($i = 0; $rv && ($i < count($a)); $i ++) {
                            $rv = $primitives($a[$i], $fl, $it);
                        }
                        // done
                    } else {
                        $k = array_keys($a); // array of the keys
                        for ($i = 0; $rv && ($i < count($a)); $i ++) {
                            if (! is_string($k[$i])) {
                                // this is case of single tag boolean
                                $rv = $rv && $primitives($a[$k[$i]], $fl, $it);
                                continue;
                            }
                            // check the scope of the key
                            $mit = $it->clone()->go_to($k[$i]);
                            if (! $mit->is_tag($k[$i])) {
                                throw new \Exception(" the key {$k[$i]} is not in schema");
                            }
                            // the item has been found
                            if (is_array($a[$k[$i]])) {
                                // the item shall be object of a kind
                                if ($mit->get()["sub"] == 0) {
                                    throw nex\Exception("{$k[$i]} is not a scope in schema");
                                }
                                // it is also a scope
                                $rv = $rv && ! $fl->p_iter->next();
                                $rv = $rv && $fl->p_iter->write_scope($mit->get()["item"]);
                                $rv = $rv && $fl->p_iter->enter_scope();
                                $rv = $rv && ! $fl->p_iter->next();
                                $rv = $rv && $primitives($a[$k[$i]], $fl, $mit);
                                $rv = $rv && ! $fl->p_iter->next();
                                $rv = $rv && $fl->p_iter->write_end();
                                $rv = $rv && $fl->p_iter->exit_scope();
                                $rv = $rv && ! $fl->p_iter->next();
                            } else {
                                // the item is not a scope
                                if ($mit->get()["sub"] > 0) {
                                    throw nex\Exception("{$k[$i]} is not a primitive in schema");
                                }
                                $rv = $rv && ! $fl->p_iter->next();
                                $rv = $rv && $fl->p_iter->write($mit->get()["item"], $a[$k[$i]], $mit->get()["type"]);
                                $rv = $rv && ! $fl->p_iter->next();
                            }
                        }
                        // done
                    }
                }
                return $rv;
            };

            $rv = $primitives($value, $fl, $it);
        }

        return $rv;
    }

    /**
     * The reference to the flat tree.
     *
     * @var array
     */
    private $p_schema = Array(
        Array(
            "sub" => 0,
            "item" => 0,
            "name" => "",
            "desc" => "",
            "type" => ""
        )
    );

    /**
     * The iterator's current index to flat tree .
     *
     * If the idx is 0 then the element has not been found, -1 if reset.
     *
     * @var integer
     */
    private int $p_idx = - 1;

    /**
     * The iterator's index to scope opener.
     * 0 is always root scope opener.
     *
     * @var integer
     */
    private int $p_scope = 0;

    /**
     * Reference to the message iterator object.
     *
     * @var Iterator
     */
    private ?Iterator $p_iter = Null;
}

?>
