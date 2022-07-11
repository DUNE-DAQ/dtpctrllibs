local moo = import "moo.jsonnet";
local ns = "dunedaq.dtpctrllibs.dtpcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {

    size: s.number("Size", "u8",
        doc="A count of very many things"),

    conf: s.record("MaskParams",[
        s.field("masks", self.vuint_data, ""),
            doc="Channel mask array"),
    ], doc="Structure for payload of DTP set masks command"),

};

s_app + moo.oschema.sort_select(cs)