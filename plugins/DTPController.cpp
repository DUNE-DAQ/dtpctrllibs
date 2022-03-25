/**
 * @file DTPController.cpp DTPController DAQModule implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

// dtpctrllibs headers
#include "DTPController.hpp"

#include "dtpctrllibs/dtpcontroller/Nljs.hpp"
#include "dtpctrllibs/dtpcontroller/Structs.hpp"

#include "dtpctrllibs/dtpcontrollerinfo/InfoNljs.hpp"
#include "dtpctrllibs/dtpcontrollerinfo/InfoStructs.hpp"

#include "dtpctrllibs/DTPIssues.hpp"

// dtpcontrols headers
#include "dtpcontrols/toolbox.hpp"

#include "uhal/uhal.hpp"

#include "logging/Logging.hpp"

// appfwk headers
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <string>
#include <vector>

namespace dunedaq {
  namespace dtpctrllibs {

    DTPController::DTPController(const std::string& name)
      : DAQModule(name)
    {
      register_command("conf", &DTPController::do_configure);
      register_command("start", &DTPController::do_start);
      register_command("stop", &DTPController::do_stop);
      register_command("reset", &DTPController::do_reset);
    }

    DTPController::~DTPController() {
      delete m_dtp_pod;
    }

    void DTPController::init(const data_t& init_data) {

    }

    void
    DTPController::do_configure(const data_t& args)
    {
      m_dtp_configuration = args.get<dtpcontroller::ConfParams>();

      // get the hardware device
      std::string conn_file = dtpcontrols::find_connection_file();

      uhal::ConnectionManager cm(conn_file, {"ipbusflx-2.0"});
      uhal::HwInterface flx = cm.getDevice(m_dtp_configuration.device);

      m_dtp_pod = dynamic_cast<const dtpcontrols::DTPPodNode*>( &flx.getNode(std::string("")) );

      // then apply TP parameters

    }

    void
    DTPController::do_start(const data_t& args) {
      const data_t args2 = args;
    }

    void
    DTPController::do_stop(const data_t& args) {
      const data_t args2 = args;
    }


    void                                                                  
    DTPController::do_reset(const data_t& args)
    {  
      const data_t args2 = args;
      m_dtp_pod->reset();
    }


    void
    DTPController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
    {
      dtpcontrollerinfo::Info module_info;
      module_info.dummy = 4;
      ci.add(module_info);
    }
    
    
  } // namespace dtpctrllibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::dtpctrllibs::DTPController)
