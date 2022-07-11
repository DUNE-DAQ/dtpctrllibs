local moo = import "moo.jsonnet";
local ns = "dunedaq.dtpctrllibs.dtpcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {

    size: s.number("Size", "u8",
        doc="A count of very many things"),

    conf: s.record("ThresholdParams",[
	s.field("threshold", self.uint_data, "20",
            doc="Trigger primitive threshold"),
    ], doc="Structure for payload of DTP set threshold command"),

};

s_app + moo.oschema.sort_select(cs)