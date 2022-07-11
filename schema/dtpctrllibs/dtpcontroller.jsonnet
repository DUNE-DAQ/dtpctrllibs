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

    double_data: s.number("DoubleData", "f8", 
         doc="A double"),

    vuint_data: s.sequence("VUintData", self.uint_data, doc="Vector of uints"),
      
    conf: s.record("ConfParams",[
        s.field("connections_file", self.str, "",
            doc="uHAL connections file"),
        s.field("device", self.str, "",
            doc="string name of managed device"),
	s.field("uhal_log_level", self.uhal_log_level, "notice",
            doc="log level for uhal. Possible values are: fatal, error, warning, notice, info, debug."),
        s.field("source", self.str, "ext",
	    doc="Data source (int or ext)"),
	s.field("threshold", self.uint_data, "20",
            doc="Trigger primitive threshold"),
        s.field("masks", self.vuint_data, ""),
    ], doc="Structure for payload of DTP configure commands"),
};

s_app + moo.oschema.sort_select(cs)