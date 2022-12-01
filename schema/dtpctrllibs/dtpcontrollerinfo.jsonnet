local moo = import "moo.jsonnet";
local s = moo.oschema.schema("dunedaq.dtpctrllibs.dtpcontrollerinfo");

local info = {

    cl : s.string("class_s", moo.re.ident, doc="A string field"), 

    uint8 : s.number("uint8", "u8", doc="An unsigned of 8 bytes"),

    stream : s.record("StreamInfo", [
        s.field("cr_if_packet_counter", self.uint8, 0, doc="packet counter from CRIF"),
    ], doc="Stream processor information")
};

moo.oschema.sort_select(info)