local moo = import "moo.jsonnet";
local ns = "dunedaq.dtpctrllibs.dtpcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {

    str : s.string("Str", doc="A string field"),
    
    size: s.number("Size", "u8",
        doc="A count of very many things"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    double_data: s.number("DoubleData", "f8", 
         doc="A double"),
      
    conf: s.record("ConfParams",[
        s.field("device", self.str, "",
            doc="String of managed device name"),
    ], doc="Structure for payload of hsi configure commands"),
};

s_app + moo.oschema.sort_select(cs)