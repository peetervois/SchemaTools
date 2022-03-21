# README of SchemaTools

Schema tools has been created to help on software communication interface 
description, interface consistency verification, message correctness verification
and automatic conversion between different formats: binary TLV, json and other 
language specific dictionaries.

Several tools exist like Protocol Buffers and others. Those systems
have some limitations or have grown over time to be overcomplex that led to 
creation of this software. Thus this does have strong influence on those 
developments, I have tried to simplify the schema language to keep it very 
flexible, small and functional. The aim is to keep it small and simple.

## Development Roadmap.

SchemaTools contains set of utils to work with schema:

 -  [x] Schema Checker (Python3)
 -  [x] Flat tree compiler,
 -  [ ] Flat tree export for C,
 -  [x] Flat tree export for PHP,
 -  [ ] Flat tree export for javascript

 -  [x] Python dict validator in Python3
 -  [ ] Binary codec for Python3
 -  [ ] Binary <-> dict for Python3

 -  [x] Binary codec for C
 -  [ ] Binary validator for C

 -  [x] Binary codec for PHP
 -  [ ] Binary <-> object converter for PHP
 -  [ ] Object validator for PHP

 -  [ ] Binary codec for javascript
 -  [ ] Binary <-> JSON for javascript
 -  [ ] JSON validator in javascript
 
 
 It is work in progress: (37%) 6/16.
 
## How to use

### Checking the schema file consistency
 
 The checker is Python3 script, you can run it, the help shows:
 
 ``` shell
 $ ./schemacheck.py -h
usage: schemacheck.py [-h] fname

Validate and process Tauria's schema files.

positional arguments:
  fname       The file name to start the schema parsing from.

optional arguments:
  -h, --help  show this help message and exit
 ```
 
 Currently the **schemacheck.py** does parse the file and build schema tree into memory. 
 At the same time it does check the consistency of the schema against the basic principles of the schema.
 
### Validating the Python dict

The method **SchemaFactory.pydict()** does verify the contents of the dictionary and remove the items
that do not match with the schema. It may raise exception when error is found, it can only print into stdout 
the error messages. It may remove the items silently. Return values will also inform the caller what happened:

**-1** Some items have been removed from the dict structure, it is ok to use it though. When the application
has been composing the dictionary, then it may be good idea to raise exception too, since it points to bug.

**0** The so called dict to verify is not trustworthy at all. Do not proceed with using it.

**1** The verification passed without changes into the dictionary.


```Python
from schemacheck import SchemaFactory

themessage = {}

factory = SchemaFactory()
factory.loadfile( "nice.schema" )
factory.pydict( themessage )

```

### API

TODO: write the api documentation
 
## Sample Schema file
 
 ```
 
# Sample schema file
 
$include another_schema_file.schema  # include the file into root name space
 
person : COLLECTION
  # this is comment line, that does describe the item person
  # another comment line to the item person
  # person defines only type alias to COLLECTION and opens new name space
   
  $include relative/path/to/schemafile.schema
   
  $include /or/absolute/path/to/schemafile.schema
person : END

this_is_item : SINT-8 = 1
  # the item is instance at root scope with instance number 1
  # 'this_is_item' is name of the item
  # 'this_is_item' is type alias to SINT-8 at the same time
  
param_id : COLLECTION
  width : BOOL = 1
  height : BOOL = 2
  depth : BOOL = 3
: END

parameters : COLLECTION
  param_id.width  # name 'width', item 1 and type BOOL is derived from param_id.width
  param_id.height : UINT-16  # name 'height' and item 2 are derived, but type is changed to UINT-16
  param_id.depth : UINT-16 = 4  # name 'depth' is derived but type and item number are changed
parameters : END

set : COLLECTION = 2
  # Remote Procedure Call 'set'
  # It is requested to set value of a parameter
  # may be encoded in JSON as
  #   { "set":{"width":True,"height":102,"depth":43} }
  parameters.width
  parameters.height
  parameters.depth
  something_else : UTF8 = 100 # note that 1,2,4 are used by width,...
  id : person.name = 101 
    # the primitive type referenced to person.name is used
    # scope reference starts from root scope
:END

get : VARIADIC = 3
  # Remote Procedure Call 'get'
  # It is requested to respond with parameter values without changing the parameters
  # may be encode in JSON as
  #   { "get":["width","depth"] }
  # or
  #   { "get":[{"width":True},{"height":False},{"depth":True}] }
  param_id.width
  param_id.height
  param_id.depth
:END

response : parameters = 4
  # The response message to 'set' and 'get' message
  # Contains accepted values for the parameters

 ```
 
## Schema Description

Idea of this schema is to be absolutely as simple and small as possible and at the same time to provide
enough flexibility to define messages for Machine to Machine communication.

### Basic principles

The element is described with following one-liner. Only multi-line item can be the comment.

```
<scope.name> : <scope.type> = <instance> # <comment>
```

- Order of the elements is not important.

- All elements are optional.

- Named item is defined when the instance number is provided and is bigger than 0. 

- Every item has type alias to its type. 
  The alias has the same name as is the elements name.
  The scope of the alias is the scope of the element.

- If scoped name or scoped type is used, they must have been declared earlier in the schema.

- Element with scoped name inherits all attributes of the referenced alias, except the comment and scope. 
  The type and instance number can be overriden.
  
- Name must be unique inside scope.

- Instance number must be unique inside scope.

- Reserved type names shall not be scoped.

 
### Primitive Types

Primitive types are types that map to computer memory model.

**BOOL** Boolean type that maps to binary information [False,True] or [0,1].

**UINT-8** Unsigned integer 8 bits. The bit lengths may vary: 8, 16, 32, 64. For example **UINT-32**.

**UINT** Unsigned integer with undefined length, concrete parser implementation shall use something big enough.

**SINT-8** Signed integer 8 bits.  The bit lengths may vary: 8, 16, 32, 64. For example **SINT-32**.

**SINT** Signed integer with undefined length, concrete parser implementation shall use something big enough.

**FLOAT-32** IEEE floating point number, The bit lengths may vary: 32, 64. For example **FLOAT-64**

**FLOAT** Alias for **FLOAT-64**.

**UTF8** Human readable string encoded in Unicode UTF-8 binary format.

**BLOB** Any kind of continuous binary data. The application shall know what it is.


### Complex Types

Complex types are collection of items with primitive or complex types.
Two complex types are defined with element that opens scope: **COLLECTION** and **VARIADIC**. 

```
<name> : COLLECTION = <instance>
```

or

```
<name> : VARIADIC = <instance>
```

Scope is closed with **END** as type name.

```
<name> : END
```
END with name must close exactly the scope with the name. To close any scope:

```
:END
```

You may wonder, why *ENUM* is excluded? It seems like the *ENUM* is not needed for this schema style.
Instead of *ENUM* use *COLLECTION* of items with type *BOOL* . 
This schema does not try to define exactly what kind of integer values are allowed to be used.
You can achieve very similar effect with the collection of booleans (see the example schema VARIADIC item 3)

#### VARIADIC

Variadic array is the most basic form of communication- a sequence of messages in mixed order and over time
the messages will repeat. The order how they appear may be important for the application.
Use VARIADIC when you need to define array of items. The root scope is of type VARIADIC. 
You may think that this wikisheet is VARIADIC array of headings and paragraphs. 

#### COLLECTION

Collection is similar to variadic array, except inside the COLLECTION items may appear only once.
Well, we can not warrant this to happen. If the element appears more than once, it is not defined which
value will be permanent.

## License
 
Copyright (c) 2021, Tauria Ltd <peeter@tauria.ee> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  *  Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.

  *  Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.

  *  Neither the name of the Tauria nor the names of its contributors 
     may be used to endorse or promote products derived from this software without 
     specific prior written permission.

THIS SOFTWARE IS PROVIDED BY TAURIA AS IS AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL TAURIA 
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE. 
