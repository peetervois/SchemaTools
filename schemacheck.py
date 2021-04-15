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
from builtins import isinstance



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
    
    def __init__(self, opened : list = None):
        self.root = SchemaItem()
        self._cur_scope = list()
        self._opened_files = list()
        if opened != None :
            self._opened_files = opened

    
    _cur_item : SchemaItem = None # the schema item currently are working with
    
    _cur_scope : list = None # the current scope of schema items currently are working with
    
    def add(self, line : str ):
        """
        Produce SchemaItem based on line of the description
        """
        #print( line )
        # remove leading and trailing whitespace
        ln1 = line.strip()
        #
        # --- check if it is empty line
        #if len( ln ) == 0 :
            # empty line
        #    self._cur_item = None # we are no longer adding comments into the item
        #    return
        #
        # --- check if we have pure comment line
        itm_cmm = ln1.split("#",1)
        ln = itm_cmm[0].strip()
        if len( ln ) == 0 and len(itm_cmm) > 1:
            # pure commentary line, add commentary to the current item if it exists
            if self._cur_item != None:
                self._cur_item.desc += "\n"+ itm_cmm[1]
            return
        #
        # --- work with the item description
        # lets derive the comments if nothing is provided
        if self._cur_item != None :
            if self._cur_item.name == "commentbase" :
                pass
            if self._cur_item.desc == "" and self._cur_item.derived != None :
                self._cur_item.desc = self._cur_item.derived.desc
        if len( ln1 ) == 0 :
            # empty line
            self._cur_item = None # we are no longer adding comments into the item
            return
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
            rootscope = True
            for i in nam :
                if len( i ) == 0 :
                    if rootscope :
                        rootscope = False
                        continue
                    raise BaseException( "error: name space type must have name; " + ln )
                rootscope = False
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
            rootscope = True
            for i in typ :
                if len( i ) == 0 :
                    if rootscope :
                        rootscope = False
                        continue
                    raise BaseException( "error: name space type must have name; " + ln )
                rootscope = False
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
            raise BaseException( "error: circular inclusion of {}".format(apath) )
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
            try:
                if len( includes ) < 2 :
                    # a line that does not contain $include statement
                    self.add(line)
                    continue
                for fn in includes :
                    fn = fn.strip()
                    if len(fn) == 0 :
                        continue
                    if fn[0] != '/' :
                        fn = dirname + "/" + fn
                    subfactory = SchemaFactory( opened=self._opened_files )
                    subfactory.loadfile(fn)
                    scop = self.root
                    if len( self._cur_scope ) > 0 :
                        scop = self._cur_scope[ len( self._cur_scope) -1 ]
                    for k, v in subfactory.root.subitems.items():
                        if k in scop.subitems :
                            raise BaseException( "error: name '{}' from include is not unique !".format( k ) )
                        for ik, i in scop.subitems.items() :
                            if v.item == i.item and v.item > 0 :
                                raise BaseException( "error: item '{}' of '{}' from include is not unique !".format(v.item,k) )
                        scop.subitems[k] = v
            except:
                e = sys.exc_info()[1]
                print( "{}:{} :: {}".format(apath,lnum,e) )
                sys.exit(1)
        self._opened_files.pop()
            

    
    def pydict(self, validate : dict, throw = False, inf = False) -> int:
        """
        Validate python dictionary against the schema.
        It does remove all items that are not described by the schema.
        
        :param validate - the dictionary to validate
        :param throw - if the method shall raise exception on removal (when True) of item 
                        or pass silently (when False)
                        
        :return 0  if the validate shall not be trusted, is not dict for example
        :return -1 if something was removed from the validate
        :return 1 if the dictionary was unchanged 
        """
        
        rv = {}
        rv['r'] = 1
        recur = list()
        
        def ex( err : str ):
            if throw:
                raise BaseException( err )
            if inf:
                print( err )
        
        
        def valueverify( key : str, val, schscope : SchemaItem ) -> bool:
            if key not in schscope.subitems :
                ex( "error: key '{}' is not found in scope '{}'".format(key, schscope.name))
                return False
            inst = schscope.subitems[key]
            if inst.item < 1 :
                # it does not belong to item
                ex( "error: key '{}' does not belong to item in scope '{}'".format(key, schscope.name))
                return False
            # verify the types of the items
            if inst.type == 'COLLECTION' :
                if not isinstance( val, dict ) :
                    ex( "error: key '{}' must be dict in scope '{}'".format(key,schscope.name))
                    return False
                valscollect( val, inst )
                if not rv['r'] :
                    ex( "error: closed recursion key '{}' identified -> breaking it under '{}'".format(key,schscope.name))
                return rv['r']
            if inst.type == 'VARIADIC' :
                if not isinstance( val, list ):
                    ex( "error: key '{}' must be list in scope '{}'".format(key,schscope.name))
                    return False
                valsvariadic( val, inst )
                if not rv['r'] :
                    ex( "error: closed recursion key '{}' identified -> breaking it under '{}'".format(key,schscope.name))
                return rv['r']
            if inst.type == 'BOOL' :
                if val != None and not isinstance( val, bool ) :
                    ex( "error: key '{}' must be '{}' in scope '{}'".format(key,inst.type,schscope.name))
                    return False
                return True
            if inst.type in ['SINT','SINT-8', 'SINT-16', 'SINT-32', 'SINT-64', 'SINT-128'] :
                if (not isinstance( val, int ) and not isinstance(val,float)) or val != int( val ):
                    ex( "error: key '{}' must be '{}' in scope '{}'".format(key,inst.type,schscope.name))
                    return False
                return True
            if inst.type in ['UINT','UINT-8', 'UINT-16', 'UINT-32', 'UINT-64', 'UINT-128'] :
                if (not isinstance( val, int ) and not isinstance( val, float )) or val < 0 or  val != int( val ):
                    ex( "error: key '{}' must be '{}' in scope '{}'".format(key,inst.type,schscope.name))
                    return False
                return True
            if inst.type in ['FLOAT', 'FLOAT-32', 'FLOAT-64', 'FLOAT-128'] :
                if not isinstance( val, int ) and not isinstance( val, float ) :
                    ex( "error: key '{}' must be '{}' in scope '{}'".format(key,inst.type,schscope.name))
                    return False
                return True
            if inst.type == 'UTF8' :
                if not isinstance( val, str ) :
                    ex( "error: key '{}' must be '{}' in scope '{}'".format(key,inst.type,schscope.name))
                    return False
                return True
            if inst.type == 'BLOB' :
                if not isinstance( val, bytes ) and not isinstance( val, str ) :
                    ex( "error: key '{}' must be '{}' in scope '{}'".format(key,inst.type,schscope.name))
                    return False
                return True
            ex( "error: key '{}' type '{}' in scope '{}' not implemented".format(key,inst.type,schscope.name))
            return False

        
        def valscollect( inscope : dict, schscope : SchemaItem ):
            if id(inscope) in recur:
                rv['r'] = 0
                return
            recur.append( id(inscope) )
            keys = list()
            for key in inscope :
                keys.append(key)
            for key in keys :
                val = inscope[key]
                if not valueverify(key,val,schscope ):
                    inscope.pop(key)
                    rv['r'] = -1
                    continue
                continue
            recur.pop()
        
        
        def valsvariadic( inscope : list, schscope : SchemaItem ):
            if id(inscope) in recur:
                rv['r'] = 0
                return
            recur.append( id(inscope) )
            i = 0
            while len(inscope) > i :
                itm = inscope[i]
                if isinstance( itm, str ):
                    # this is special case of object inside variadic array that containing single boolean with True value
                    if not valueverify(itm,None,schscope ):
                        inscope.pop(i)
                        rv['r'] = -1
                        continue
                    i += 1
                    continue 
                if isinstance( itm, dict ):
                    # the other option is that the item is dictionary
                    valscollect( itm, schscope )    
                    if len( itm ) < 1 :
                        inscope.pop(i)
                        rv['r'] = -1
                        continue
                    i += 1
                    continue
                # this is something dangerous, do must be removed
                ex( "error: item '{}' is not string nor dict inside variadic '{}'".format(itm,schscope.name))
                rv['r'] = -1
                inscope.pop(i)
                continue            
            recur.pop()
        
        if not isinstance( validate, dict ) :
            ex( "error: dictionary not provided to the validation !")
            return 0
        valscollect( validate, self.root )
        return rv['r']
    
    
    def generate_flat_tree(self) -> dict:
        """
        Flat tree is needed for other language support compilation. It provides generic model of 
        the schema structure
        """
        
        tree = []               # tree is flat list.

        def new_row( v : SchemaItem ) -> dict:
            rv = { 
                "item": v.item,          # the item identificator number
                "name": v.name,          # the item name
                "type": v.type,          # the type name, contains only primitives
                "desc": v.desc,          # description for the engineers
                "sub": 0,                # index to first subitem
                "next": 0,               # index to the next item at the same scope
                "_idx": len(tree)        # its flatrow index
            }
            tree.append(rv)
            return rv
        
        
        if getattr(self.root,'_flatrow',None) == None :
            # create flatrow for the root item
            # and place it as the item 0
            self.root._flatrow = new_row(self.root)
            self.root._flattree = tree
        else:
            # the flatrow actually does exist already
            return self.root._flattree
        
        
        def scan_subitems( itm : SchemaItem ):
            if itm._flatrow['sub'] > 0 :
                # the item's subitems have been already scanned or is in the middle of scanning
                return
            for k,v in itm.subitems.items():
                if v.item < 1 :
                    # we take only existing items
                    continue
                if getattr(v,'_flatrow',None) == None:
                    # the item does not have yet the flat row
                    v._flatrow = new_row(v)
                    if len(tree) > 6 :
                        pass
                if itm._flatrow['sub'] == 0:
                    itm._flatrow['sub'] = v._flatrow['_idx']
                else:
                    i = itm._flatrow['sub']
                    j = i
                    while i > 0 :
                        j = i
                        i = tree[i]['next']
                    tree[j]['next'] = v._flatrow['_idx']
                if len( v.subitems ) > 0:
                    scan_subitems( v )
                if v._flatrow['next'] > 0:
                    break
                pass
            pass
        
        scan_subitems(self.root)
                    
        return self.root._flattree
        
    pass
    
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Validate and process Tauria\'s schema files.')
    parser.add_argument('fname', type = str, nargs=1, help="The file name to start the schema parsing from.")
    args = parser.parse_args()
    
    factory = SchemaFactory()
    
    factory.loadfile( args.fname[0] )
    
            
    print( 'done' )
        
    
    