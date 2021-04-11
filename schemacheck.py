#!/usr/bin/python3
#
# this program does load schema file and check it
#
#
# Copyright (c) 2021, Tauria Ltd <peeter@tauria.ee> All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
#  *  Redistributions of source code must retain the above copyright notice, this 
#     list of conditions and the following disclaimer.
#
#  *  Redistributions in binary form must reproduce the above copyright notice, 
#     this list of conditions and the following disclaimer in the documentation 
#     and/or other materials provided with the distribution.
#
#  *  Neither the name of the Tauria nor the names of its contributors 
#     may be used to endorse or promote products derived from this software without 
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY TAURIA AS IS AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TAURIA 
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE. 



import posixpath
import argparse
import sys



class SchemaItem:
    """
    SchemaItem does hold information about the Schema item
    
    [<scope>.[<scope>.[...]]]<name> [: [<scope>.[<scope>.[...]]]<type>] [= <instance>]
    
    special primitive types: BOOL, INT[-<bits>], UINT[-<bits>], FLOAT[-<bits>], UTF8
    special complex types: COLLECTION, VARIADIC 
    """
    
    name : str  = ""
    """
    The name of the item. It also is the name of the type of the item
    """
    
    name_scope : list = None
    """
    list of SchemaItem of provided scope in name field
    """
    
    type : str = ""
    """
    The name of the type this item is alias for
    """
    
    type_scope : list = None
    """
    List of SchemItem of provided scope in the type field. Does also contain reference to
    the type this item is alias for
    """
        
    item : int = 0
    """
    The item number, it must be non-zero when the item exists, type of the item does
    exist always. The value of the item has to be unique
    """
    
    desc : str = ""
    """
    The human understandable description of the item
    """
    
    derived = None
    """
    The reference  to SchemaItem this item is derived from.
    Deriving works by providing scope path for the name of the item.
    When deriving, first all attributes from the parent are taken,
      then the attributes are overriden with the specification on
      the current item.
    """
    
    subitems : dict = None
    """
    The SchemaItems that are encapsulated into this complex object if any
    """
    
    def __init__(self):
        self.name_scope = list()
        self.type_scope = list()
        self.subitems = dict()
        
    

class SchemaFactory:
    """
    Class that does handle parsing of schema, and that does produce SchemaItems
    """
    
    root : SchemaItem = None
    """
    The Schema root item
    """
    
    def __init__(self):
        self.root = SchemaItem()
        self._cur_scope = list()
        self._opened_files = list()
    
    _cur_item : SchemaItem = None # the schema item currently are working with
    
    _cur_scope : list = None # the current scope of schema items currently are working with
    
    def add(self, line : str ):
        """
        Produce SchemaItem based on line of the description
        """
        #print( line )
        # remove leading and trailing whitespace
        ln = line.strip()
        #
        # --- check if it is empty line
        if len( ln ) == 0 :
            # empty line
            self._cur_item = None # we are no longer adding comments into the item
            return
        #
        # --- check if we have pure comment line
        itm_cmm = ln.split("#",1)
        ln = itm_cmm[0].strip()
        if len( ln ) == 0 :
            # pure commentary line, add commentary to the current item if it exists
            if self._cur_item != None:
                self._cur_item.desc += "\n"+ itm_cmm[1]
            return
        #
        # --- work with the item description
        # the previous item is closed now
        self._cur_item = None
        # split the line by ':'
        nam_typ = ln.split(":")
        if len( nam_typ ) > 2 :
            raise BaseException( "error: more than one ':'; " + ln )
        if len( nam_typ ) < 2 :
            # type specification is missing, try to reuse the type
            nam_typ = ln.split("=")
            if len(nam_typ) > 1 :
                nam_typ[1] = "=" + nam_typ[1]
            else:
                nam_typ.append('')
        # split the name part
        nam = nam_typ[0].strip().split(".") # nam is list of namespace names 
        # split the line by '='
        typ_num = nam_typ[1].split("=")
        if len(typ_num) > 2 :
            raise BaseException( "error: more than one '='; " + ln )
        typ = typ_num[0].strip().split(".") # typ is list of namespace types
        num = 0 # num is the instance number, 0 means not set
        if len(typ_num) > 1 :
            num = int( typ_num[1] )
        scop = self.root
        for scop in self._cur_scope : pass # scop is current scope
        #
        # --- check for END, closure of the namespace
        if "END" in typ :
            # we have closing scope,
            if len( typ ) > 1 :
                raise BaseException( "error: name space of types not allowed with END; " + ln )
            if len( self._cur_scope ) < 1 :
                raise BaseException( "error: unmatched END; " + ln )
            if len( nam ) > 1 :
                raise BaseException( "error: name space not allowed with END; " + ln )
            if len( nam[0] ) > 0 and scop.name != nam[0] :
                raise BaseException( "error: unmatched end of current scope {}; {}".format( scop.name, ln) )
            self._cur_scope.pop()
            return
        #
        # --- create the item we are working with
        # find parent type
        nnam = len(nam)
        if len( nam[nnam-1] ) == 0 :
            raise BaseException( "error: item must have name part; " + ln )
        self._cur_item = SchemaItem()
        if len(itm_cmm) > 1 :
            cmm = itm_cmm[1].rstrip()
            if len(cmm) > 0 :
                self._cur_item.desc = itm_cmm[1].rstrip()
        if len( nam ) > 1 :
            # find the parent type
            k = self.root
            for i in nam :
                if len( i ) == 0 :
                    raise BaseException( "error: name space type must have name; " + ln )
                if i not in k.subitems :
                    raise BaseException( "error: name '{}' not found; {}".format(i,ln) )
                k = k.subitems[i]
            # we have found the parent type
            self._cur_item.derived = k
            if len( typ ) == 1 and len( typ[0] ) == 0 :
                # we use another typename, it is the same as name
                typ.clear()
                for i in nam :
                    typ.append( i )
        # place the name 
        self._cur_item.name = nam.pop()
        # place the scope
        for i in self._cur_scope :
            self._cur_item.name_scope.append(i)
        # check the name uniqueness
        if self._cur_item.name in scop.subitems :
            raise BaseException( "error: name {} is not unique; {}".format( self._cur_item.name, ln ) )
        #
        # working with instance number
        if num == 0 and self._cur_item.derived != None :
            # the instance number is not set take the derived instance number
            self._cur_item.item = self._cur_item.derived.item
        else :
            self._cur_item.item = num
        if self._cur_item.item < 0 :
            raise BaseException( "error: the instance must be positive number; " + ln )
        for i,v in scop.subitems.items() :
            if v.item > 0 and v.item == self._cur_item.item :
                raise BaseException( "error: the instance {} is not unique in scope; {}".format( v.item, ln ) )
        # register the  new item into current scope
        scop.subitems[self._cur_item.name] = self._cur_item            
        #
        # working with type
        if "COLLECTION" in typ or "VARIADIC" in typ :
            # we have opening scope
            if len( typ ) > 1 :
                raise BaseException( "error: name space of types not allowed with complex types; " + ln )
            self._cur_scope.append( self._cur_item )
            self._cur_item.type = typ[0]
            return
        if len( typ ) == 0 :
            # we are using derived type
            if self._cur_item.derived == None :
                raise BaseException( "error: type must be specified; " + ln )
            self._cur_item.type = self._cur_item.derived.type
            self._cur_item.type_scope = self._cur_item.derived.type_scope
            return
        primitives = ["BOOL", "SINT", "UINT", "UTF8","BLOB","FLOAT",
                      "SINT-8","SINT-16","SINT-32","SINT-64","SINT-128",
                      "UINT-8","UINT-16","UINT-32","UINT-64","UINT-128",
                      "FLOAT-32","FLOAT-64","FLOAT-128"]
        primitive = None
        for i in primitives :
            for j in typ :
                if i == j :
                    # we have primitive in types
                    if len(typ) > 1 :
                        raise BaseException( "error: name space of types not allowed with primitives; " + ln )
                    primitive = i
        # now, when primitive is used, the scope of types does not exist
        self._cur_item.type = ""
        self._cur_item.type_scope = list()
        if primitive == None :
            # we have scoped type
            # find the parent type
            k = self.root
            for i in typ :
                if len( i ) == 0 :
                    raise BaseException( "error: name space type must have type; " + ln )
                if i not in k.subitems :
                    raise BaseException( "error: type '{}' not found; {}".format(i,ln) )
                k = k.subitems[i]
                self._cur_item.type_scope.append(k)
            # expand the type
            self._cur_item.subitems = k.subitems
            self._cur_item.type = k.type
        else :
            self._cur_item.type = primitive 
                
        # DONE !!! 
    
    _opened_files : list = None
    
    def loadfile(self, path : str):
        """
        Load schema from file
        """
        # check if the file is already opened and not yet closed
        apath = posixpath.abspath(path)
        if apath in self._opened_files :
            return
        self._opened_files.append(apath)
        dirname = posixpath.dirname( apath )
        #print(" --- {}".format(apath))
        f = open( apath, "r" )
        lnum = 0
        while True:
            line = f.readline()
            if len(line) == 0 :
                break
            lnum += 1
            includes = line.split("#",1)[0].split("$include")
            if len( includes ) < 2 :
                # a line that does not contain $include statement
                try:
                    self.add(line)
                except:
                    e = sys.exc_info()[1]
                    print( "{}:{} :: {}".format(apath,lnum,e) )
                continue
            for fn in includes :
                fn = fn.strip()
                if len(fn) == 0 :
                    continue
                if fn[0] != '/' :
                    fn = dirname + "/" + fn
                self.loadfile(fn)
        self._opened_files.pop()
            
        

    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Validate and process Tauria\'s schema files.')
    parser.add_argument('fname', type = str, nargs=1, help="The file name to start the schema parsing from.")
    args = parser.parse_args()
    
    factory = SchemaFactory()
    
    factory.loadfile( args.fname[0] )
    
            
    print( 'done' )
        
    
    