local moo = import "moo.jsonnet";
local ns = "dunedaq.dtpctrllibs.dtpcontroller";
local s = moo.oschema.schema(ns);

local s_app = import "appfwk/app.jsonnet";
local app = moo.oschema.hier(s_app).dunedaq.appfwk.app;

local cs = {

    str : s.string("Str", doc="A string field"),
    
    uhal_log_level : s.string("UHALLogLevel", pattern=moo.re.ident_only,
                    doc="Log level for uhal. Possible values are: fatal, error, warning, notice, info, debug."),

    size: s.number("Size", "u8",
        doc="A count of very many things"),

    uint_data: s.number("UintData", "u4",
        doc="A count of very many things"),

    arr_uint_data: s.sequence("ArrUintData", cs.uint_data,
        doc="An array of values"),

    double_data: s.number("DoubleData", "f8", 
         doc="A double"),

    conf: s.record("ConfParams",[
        s.field("connections_file", self.str, "",
            doc="uHAL connections file"),
        s.field("device", self.str, "",
            doc="String name of managed device"),
	s.field("uhal_log_level", self.uhal_log_level, "notice",
            doc="Log level for uhal. Possible values are: fatal, error, warning, notice, info, debug."),
        s.field("source", self.str, "ext",
	    doc="Data source (int or ext)"),
        s.field("pattern", self.str, "",
            doc="Path to pattern file (data source = int)"),
	s.field("threshold", self.uint_data, "20",
            doc="Trigger primitive threshold"),
	s.field("masks", self.arr_uint_data, [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
            doc="Trigger primitive threshold"),
    ], doc="Structure for payload of DTP configure commands"),

};

s_app + moo.oschema.sort_select(cs)