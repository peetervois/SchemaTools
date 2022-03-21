<?php

namespace tauschema_flattree;

class device_info
{
  public static $schema=array(
    array("item"=>0,"name"=>"","type"=>"","desc"=>"","sub"=>1,"next"=>-1),
    array("item"=>1,"name"=>"info","type"=>"COLLECTION","desc"=>"\n\n Client asks for the device information. Provide the value items\n with content all bytes 0x00. Into response the data may\n be filled on site.\n\n Server does respond to the get request. Contains only\n requested items if exist. The response may be filled\n on site. When the item does not exist, then the field\n may be replaced with stuffing T0?0. When blobs contain\n less data than field has, then remainder is turned into\n stuffing with T0?0. Thus the slice blob length tells amount\n of returned data.\n\n Description of the device information.\n","sub"=>2,"next"=>0),
    array("item"=>1,"name"=>"name","type"=>"COLLECTION","desc"=>" UTF8, Device name.","sub"=>3,"next"=>6),
    array("item"=>2,"name"=>"orig","type"=>"UINT-32","desc"=>" The origin of the blob from the full data,\n   count as 0 if omitted.","sub"=>0,"next"=>4),
    array("item"=>3,"name"=>"size","type"=>"UINT-16","desc"=>" Response total number of bytes in requested\n   elements. If size is not requested then\n   it is not returned","sub"=>0,"next"=>5),
    array("item"=>1,"name"=>"data","type"=>"BLOB","desc"=>" The data slice returned. On request the\n   value field is stuffed with zeroes.\n   On response it is filled with data.","sub"=>0,"next"=>0),
    array("item"=>8,"name"=>"msglen","type"=>"UINT-32","desc"=>" {req} maximal supported length of message.","sub"=>0,"next"=>7),
    array("item"=>2,"name"=>"version","type"=>"COLLECTION","desc"=>" UTF8, Device version.","sub"=>3,"next"=>8),
    array("item"=>3,"name"=>"serial","type"=>"COLLECTION","desc"=>" UTF8, Device serial code.","sub"=>3,"next"=>9),
    array("item"=>4,"name"=>"vendor","type"=>"COLLECTION","desc"=>" UTF8, Device vendor information.","sub"=>3,"next"=>10),
    array("item"=>5,"name"=>"schtxt","type"=>"COLLECTION","desc"=>" UTF8, The schema as text file.","sub"=>3,"next"=>11),
    array("item"=>6,"name"=>"schurl","type"=>"COLLECTION","desc"=>" UTF8, Link to the schema URL.","sub"=>3,"next"=>12),
    array("item"=>7,"name"=>"schbin","type"=>"VARIADIC","desc"=>" The array of schema flat tree.","sub"=>13,"next"=>0),
    array("item"=>1,"name"=>"schrow","type"=>"COLLECTION","desc"=>" Row description","sub"=>14,"next"=>0),
    array("item"=>1,"name"=>"item","type"=>"UINT-16","desc"=>" {req} Item number in the binary.","sub"=>0,"next"=>15),
    array("item"=>2,"name"=>"name","type"=>"COLLECTION","desc"=>" {req} UTF8, Item name in the code\n   and json.","sub"=>3,"next"=>16),
    array("item"=>3,"name"=>"desc","type"=>"COLLECTION","desc"=>" UTF8, Description of the item for\n   engineer to read.","sub"=>3,"next"=>17),
    array("item"=>4,"name"=>"type","type"=>"UINT-8","desc"=>" {req} Type number of the item.","sub"=>0,"next"=>18),
    array("item"=>5,"name"=>"sub","type"=>"UINT-16","desc"=>" {req} Flat tree index to first subitem\n   of this item (COLLECTION and VARIADIC).","sub"=>0,"next"=>19),
    array("item"=>6,"name"=>"next","type"=>"UINT-16","desc"=>" {req} Flat tree index to next item at the\n   same scope.","sub"=>0,"next"=>20),
    array("item"=>7,"name"=>"idx","type"=>"UINT-16","desc"=>" Index of the row, on request and\n   response, if omitted consider as 0.","sub"=>0,"next"=>0)
  );
}

?>
