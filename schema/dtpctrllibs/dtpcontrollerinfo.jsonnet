local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.dtpctrllibs.dtpcontrollerinfo");

local info = {

    cl : s.string("class_s", moo.re.ident, doc="A string field"), 

    uint8 : s.number("uint8", "u8", doc="An unsigned of 8 bytes"),

    array : s.sequence("Array", self.uint8, doc="a list of unsigned 8 bytes"),

    stream_proc : s.record("StreamProc",[
        s.field("cr_if_packet_counter", self.uint8, 0, doc="packet counter from CRIF"),
    ], doc="stream processor"
    ),

    link_proc : s.sequence("LinkProc", self.stream_proc, doc="link processor"),
    link_procs: s.sequence("LinkProcs", self.link_proc, doc="link processors"),

    info: s.record("Info",[
        s.field("dummy", self.uint8, "",
            doc="Dummy info"),
        s.field("link_procs", self.link_procs, doc="CRIF information.")
    ], doc="DTPController information"),
};

moo.oschema.sort_select(info)