

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
    }]
    
    for i in tests :
        if 'dc' in i :
            print( " --- " + i['dc'] )
        print( i['sc'] )
        print( factory.pydict(i['sc'], inf=True) == i['rv'] )
        print( i['sc'] )
        print()
    


