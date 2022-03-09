/**
 * @file DTPController.cpp DTPController DAQModule implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#include "DTPController.hpp"

#include "dtpcontrols/toolbox.hpp"

#include "uhal/uhal.hpp"

#include "logging/Logging.hpp"

#include <nlohmann/json.hpp>

#include <bitset>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
  namespace dtpctrllibs {

    DTPController::DTPController(const std::string& name)
      : DAQModule(name)
    {
      register_command("conf", &DTPController::do_configure);
      register_command("reset", &DTPController::do_reset);
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
    DTPController::do_reset(const data_t& args)
    {  
      m_dtp_pod->reset();
    }


    //    void
    //    DTPController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
    //    {
    //      dtpcontrollerinfo::ChannelInfo info;  
    //    }
    
    
  }
}

DEFINE_DUNE_DAQ_MODULE(dunedaq::dtpctrllibs::DTPController)
