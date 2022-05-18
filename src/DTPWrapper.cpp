
/**
 * @file DTPWrapper.cpp 
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
// From Module
#include "DTPWrapper.hpp"

#include "logging/Logging.hpp"

// from dtpcontrols
#include "dtpcontrols/toolbox.hpp"

#include "uhal/uhal.hpp"

// From STD
#include <chrono>
#include <memory>
#include <string>

namespace dunedaq::dtpctrllibs {

  DTPWrapper::DTPWrapper() { }

  DTPWrapper::~DTPWrapper() { 
    delete m_dtp_pod_node;
  }

  void
  DTPWrapper::init(const data_t& /* args */) {

    std::string device("flx-0-p2-hf");
    std::string conn_file = dtpcontrols::find_connection_file();
    
    uhal::ConnectionManager cm(conn_file, {"ipbusflx-2.0"});
    uhal::HwInterface flx = cm.getDevice(device);
    
    m_dtp_pod_node = dynamic_cast<const dtpcontrols::DTPPodNode*>( &flx.getNode(std::string("")) );
    
  }

  void
  DTPWrapper::reset() {
    m_dtp_pod_node->reset();
  }

  void
  DTPWrapper::configure(const data_t& /* args */) {

  }
  
}
