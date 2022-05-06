cmake_minimum_required(VERSION 3.12)
project(dtpctrllibs VERSION 1.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(appfwk REQUIRED)
find_package(ers REQUIRED)     
find_package(logging REQUIRED)
find_package(opmonlib REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(uhal REQUIRED)
find_package(pugixml REQUIRED)

find_package(dtpcontrols REQUIRED)

daq_codegen( dtpcontroller.jsonnet
	     DEP_PKGS appfwk iomanager rcif cmdlib networkmanager TEMPLATES Structs.hpp.j2 Nljs.hpp.j2)

daq_codegen( dtpcontrollerinfo.jsonnet
	     DEP_PKGS opmonlib TEMPLATES opmonlib/InfoStructs.hpp.j2 opmonlib/InfoNljs.hpp.j2 )

set(DTPCTRLLIBS_DEPENDENCIES
	appfwk::appfwk
	ers::ers
  	logging::logging
  	opmonlib::opmonlib
  	nlohmann_json::nlohmann_json
	uhal::uhal
	uhallibs::uhallibs
  	dtpcontrols::dtpcontrols
)

##############################################################################
daq_add_library(DTPWrapper.cpp LINK_LIBRARIES ${DTPCTRLLIBS_DEPENDENCIES})

# plugins
daq_add_plugin(DTPController duneDAQModule LINK_LIBRARIES dtpcontrols::dtpcontrols dtpctrllibs uhal::uhal pugixml::pugixml)

###############################################################################
#daq_add_python_bindings(toy_wrapper.cpp LINK_LIBRARIES ${PROJECT_NAME})

##############################################################################
#daq_add_application( toylibrary_test_program toylibrary_test_program.cxx TEST LINK_LIBRARIES ${Boost_PROGRAM_OPTIONS_LIBRARY} toylibrary )

##############################################################################
#daq_add_unit_test(ValueWrapper_test)

##############################################################################

# Could add "toylibrary_test_program" to the argument list here, but
# presumably end users wouldn't care about an integration test program
# and just want the shared object library installed

daq_install()



