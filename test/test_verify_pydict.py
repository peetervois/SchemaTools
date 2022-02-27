

from schemacheck import SchemaFactory



if __name__ == "__main__":

    factory = SchemaFactory()
    
    factory.loadfile( "nice.schema" )
    
    tests = [{
        'rv':1,
        'sc':{'set':{ 
                'parameters':{ 
                    'width': 2,
                    'height': 2.1,
                    'depth': -1  
                }
            }}
    },{
        'rv':-1,
        'sc':{'par_id':{ 'width':-2.3 }}
    },{
        'rv':0,
        'sc':[1,2,3]
    },{
        'rv':1,
        'sc':{'bits':["allright",{"another":True}]}
    },{
        'rv':1,
        'sc':{'bits':["allright",{"another":True}]}
    },{
        'rv':-1,
        'sc':{'bits':["allright",{"another":True},"notright"]}
    },{
        'rv':-1,
        'sc':{'bits':{"allright":True}}
    },{
        'rv':-1,
        'sc':{'bits':["allright",1]}
    },{
        'rv':-1,
        'sc':{'nohh':'thisdoesnot exist'}
    },{
        'rv':-1,
        'sc':{'set':["notexist"]}
    },{
        'rv':-1,
        'sc':{'bits':["allright",{"another":3}]}
    },{
        'dc':"test of any kind of SINT and SINT-X",
        'rv':1,
        'sc':{'sints':{'sint':-1,'sint8':1.0,'sint16':34,'sint32':-22,'sint64':66,'sint128':-23}}
    },{
        'dc':"noninteger floating value must be catched ",
        'rv':-1,
        'sc':{'sints':{'sint':2.3}}
    },{
        'dc':"string value must be catched ",
        'rv':-1,
        'sc':{'sints':{'sint':"ahaa"}}
    },{
        'dc':"test of any kind of UINT and UINT-X",
        'rv':1,
        'sc':{'uints':{'uint':1,'uint8':1.0,'uint16':34,'uint32':22,'uint64':66,'uint128':0}}
    },{
        'dc':"noninteger floating value must be catched ",
        'rv':-1,
        'sc':{'uints':{'uint':2.3}}
    },{
        'dc':"string value must be catched ",
        'rv':-1,
        'sc':{'uints':{'uint':"1"}}
    },{
        'dc':"negative value must be catched ",
        'rv':-1,
        'sc':{'uints':{'uint':-4}}
    },{
        'dc':"test of any kind of FLOAT and FLOAT-X",
        'rv':1,
        'sc':{'floats':{'float':1,'float32':22,'float64':66.3,'float128':1.34e5}}
    },{
        'dc':"string value must be catched ",
        'rv':-1,
        'sc':{'floats':{'float':"ahaa"}}
    },{
        'dc':"test of UTF8",
        'rv':1,
        'sc':{'utf8':"Ärge Ütelge šeda €vs$"}
    },{
        'dc':"number must be catched",
        'rv':-1,
        'sc':{'utf8':45}
    },{
        'dc':"testing blob as string and bytestring",
        'rv':1,
        'sc':{'blob':{'b1':b'aksdh fhskjfhkdf\x00aksjdakf', 'b2':"alskdjalsfhaksf"}}
    },{
        'dc':"number must be catched",
        'rv':-1,
        'sc':{'blob':{'b1':2}}
    },{
        'dc':"testing None type, must pass",
        'rv':1,
        'sc':{'blob':{'b1':None}}
    },{
        'dc':"test if root object under variadic has more than one item",
        'rv':-1,
        'sc':{'bits':['allright',{'allright':True},{'another':True,'errors':False}]}
    },{
        'dc':"test if root object under variadic has more than one item #1",
        'rv':1,
        'sc':{'bits':['allright',{'allright':True},{'another':True},{'errors':False}]}
    },{
        'dc':"test multiple root scope elements",
        'rv':-1,
        'sc':{'sints':{'sint':-10},'uints':{'uint':20}}
    }]
    
    circ = { "before":11, "after":33 }
    circ["circular"] = circ
    tests.append({'dc':'making circular test','rv':-1,'sc':{'circular':circ}})

    circ1 = { "before":11, "after":33 }
    circ2 = { "before":11, "after":33 }
    circ3 = { "before":11, "circular":circ1, "after":33 }
    circ1["circular"] = circ2
    circ2["circular"] = circ3
    tests.append({'dc':'making depth 3 circular test','rv':-1,'sc':{'circular':circ1}})
    
    circ1v = { "before":11, "varcirc":[], "after":33 }
    circ2v = { "before":11, "varcirc":[], "after":33 }
    circ3v = { "before":11, "varcirc":[], "after":33 }
    circ1v["varcirc"].append({ "circular":circ2v})
    circ2v["varcirc"].append({ "circular":circ3v})
    circ3v["varcirc"].append({ "circular":circ1v})
    tests.append({'dc':'making depth 3 variadic circular test','rv':-1,'sc':{'circular':circ1v}})
    
    
    def verify_comment_derived( fact : SchemaFactory ) -> bool:
        if fact.root.subitems['derivecomment'].subitems['commentbase'].desc != "\nwithcomment" :
            print("error: comment not derived")
            return False
        return True
    
    tests.extend([{
        'dc':"deriving comments",
        'fn':verify_comment_derived
    }])
    
    def verify_flattree( fact : SchemaFactory ) -> bool:
        tree = fact.generate_flat_tree()
        for i in tree :
            print( i )
        return True

    tests.extend([{
        'dc':"producing flat tree",
        'fn':verify_flattree
    }])

    
    num_tests = 0
    num_fails = 0
    for i in tests :
        num_tests += 1
        #if num_tests == 26 :
        #    print(" do break ")
        if 'dc' in i :
            print( " --- " + i['dc'] )
        if 'sc' in i :
            print( i['sc'] )
            if factory.pydict(i['sc'], inf=True) == i['rv'] :
                print( "Passed" )
            else:
                print( "  ***  F A I L E D   ***  ")
                num_fails += 1
            print( i['sc'] )
        if 'fn' in i :
            if i['fn']( factory ) :
                print( "Passed" )
            else:
                print( "  ***  F A I L E D   ***  ")
                num_fails += 1
        print()
    
    print(" --- testing partial schema")
    partfactory = SchemaFactory()
    partfactory.loadfile( "partial.schema" )
    verify_flattree( partfactory )
    print()
    
    print(" --- testing php flat tree ")
    print( partfactory.produce_php_flattree() )
        
        
    print()
    print(     "--------------------------------")
    print(     "     Number of tests ran: {}".format(num_tests))
    print(     "  Number of tests failed: {}".format(num_fails))
    if num_fails :
        print( "                          ^^^^^")
    print(     "--------------------------------")


