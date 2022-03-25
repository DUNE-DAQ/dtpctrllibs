local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.dtpctrllibs.dtpcontrollerinfo");

local info = {

    cl : s.string("class_s", moo.re.ident,
                  doc="A string field"), 

    uint8  : s.number("uint8", "u8",
                     doc="An unsigned of 8 bytes"),

    info: s.record("Info",[
        s.field("dummy", self.uint8, "",
            doc="Dummy info"),
    ], doc="DTPController information"),
};

moo.oschema.sort_select(info)