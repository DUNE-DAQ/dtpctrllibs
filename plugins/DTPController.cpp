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

// dtpcontrols headers
#include "dtpcontrols/toolbox.hpp"
#include "uhal/ConnectionManager.hpp"
#include "uhal/log/exception.hpp"
#include "uhal/utilities/files.hpp"

// appfwk headers
#include <string>
#include <vector>

#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#define TLVL_INFO 10

namespace dunedaq {

ERS_DECLARE_ISSUE(dtpctrllibs, InvalidUHALLogLevel, "Invalid UHAL log level supplied: " << log_level, ((std::string)log_level))
ERS_DECLARE_ISSUE(dtpctrllibs, UHALConnectionsFileIssue, "Connection file not found : " << filename, ((std::string)filename))
ERS_DECLARE_ISSUE(dtpctrllibs, UHALDeviceNameIssue, "Device name not found : " << devicename, ((std::string)devicename))
ERS_DECLARE_ISSUE(dtpctrllibs, ModuleNotConfigured, "Cannot complete operation since " << name << "is not configured", ((std::string)name))

namespace dtpctrllibs {

//-----------------------------------------------------------------------------
DTPController::DTPController(const std::string& name)
    : DAQModule(name) {
  register_command("conf", &DTPController::do_configure);
  register_command("start", &DTPController::do_start);
  register_command("stop", &DTPController::do_stop);
  register_command("reset", &DTPController::do_reset);
}

//-----------------------------------------------------------------------------
DTPController::~DTPController() {
}

//-----------------------------------------------------------------------------
void DTPController::init(const data_t& /* init_data */) {
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": init() method does nothing";
}


//-----------------------------------------------------------------------------
void DTPController::do_configure(const data_t& args) {
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_configure() method";

  m_dtp_cfg = args.get<dtpcontroller::Conf>();

  // set UHAL log level
  if (m_dtp_cfg.uhal_log_level.compare("debug") == 0) {
    uhal::setLogLevelTo(uhal::Debug());
  } else if (m_dtp_cfg.uhal_log_level.compare("info") == 0) {
    uhal::setLogLevelTo(uhal::Info());
  } else if (m_dtp_cfg.uhal_log_level.compare("notice") == 0) {
    uhal::setLogLevelTo(uhal::Notice());
  } else if (m_dtp_cfg.uhal_log_level.compare("warning") == 0) {
    uhal::setLogLevelTo(uhal::Warning());
  } else if (m_dtp_cfg.uhal_log_level.compare("error") == 0) {
    uhal::setLogLevelTo(uhal::Error());
  } else if (m_dtp_cfg.uhal_log_level.compare("fatal") == 0) {
    uhal::setLogLevelTo(uhal::Fatal());
  } else {
    throw InvalidUHALLogLevel(ERS_HERE, m_dtp_cfg.uhal_log_level);
  }

  // get the connections
  std::vector<std::string> pcols = {"ipbusflx-2.0"};
  std::string conn_file("file://");
  conn_file += m_dtp_cfg.connections_file;
  std::unique_ptr<uhal::ConnectionManager> cm;
  try {
    cm = std::make_unique<uhal::ConnectionManager>(conn_file, pcols);
  } catch (const uhal::exception::FileNotFound& excpt) {
    throw UHALConnectionsFileIssue(ERS_HERE, conn_file, excpt);
  }

  // get the device
  auto devices = cm->getDevices();
  if (std::find(devices.begin(), devices.end(), m_dtp_cfg.device) == devices.end()) {
    throw UHALDeviceNameIssue(ERS_HERE, m_dtp_cfg.device);
  }

  m_pod = std::make_unique<dtpcontrols::DTPPodController>(cm->getDevice(m_dtp_cfg.device));

  // here we will need to setup the FW config
  // ie. number of links, number of pipes etc.
  uint n_links = m_pod->get_n_links();
  uint n_streams = m_pod->get_n_streams();

  //
  // RESET
  //
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": resetting DTP pod";
  m_pod->reset();

  //
  // CRIF
  //
  // set CRIF to drop empty
  for (uint i_link = 0; i_link < n_links; ++i_link) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting CRIF to drop empy packets for link processor " << i_link;
    for (uint i_stream = 0; i_stream < n_streams; ++i_stream) {
      m_pod->get_link_processor_node(i_link).get_central_router_node(i_stream).set_drop_empty();
    }
  }
  // TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting CRIF to drop empy packets";
  // m_pod->get_crif_node().set_drop_empty();

  // enable CRIF
  for (uint i_link = 0; i_link < n_links; ++i_link) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": enabling CRIF output for link processor " << i_link;
    for (uint i_stream = 0; i_stream < n_streams; ++i_stream) {
      m_pod->get_link_processor_node(i_link).get_central_router_node(i_stream).enable();
    }
  }
  // TLOG_DEBUG(TLVL_INFO) << get_name() << ": enabling CRIF output";
  // m_pod->get_crif_node().enable();


  //
  // DATAFLOW
  //
  // set source
  if (m_dtp_cfg.source == "ext") {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting input to GBT";
    m_pod->get_flowmaster_node().set_source_gbt();
  } else {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting input to wibulator";
    m_pod->get_flowmaster_node().set_source_wtor();
    for (uint i = 0; i < n_links; ++i) {
      auto data = dunedaq::dtpcontrols::read_WIB_pattern_from_file(m_dtp_cfg.pattern);
      m_pod->get_wibulator_node(i).write_pattern(data);
    }
  }

  // set sink
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting sink to hits";
  m_pod->get_flowmaster_node().set_sink_hits();

  //
  // HITFINDER CHAIN
  // 
  // set threshold
  for (uint i_link = 0; i_link < n_links; ++i_link) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting threshold for link processor " << i_link << " to " << m_dtp_cfg.threshold;
    for (uint i_stream = 0; i_stream < n_streams; ++i_stream) {
      m_pod->get_link_processor_node(i_link).set_threshold(i_stream, m_dtp_cfg.threshold);
    }
  }

  // set masks
  int masks_size = 20;  // m_dtp_cfg.masks.size();
  if (masks_size == n_links * n_streams) {
    for (uint i_link = 0; i_link < n_links; i_link++) {
      for (uint i_stream = 0; i_stream < n_streams; i_stream++) {
        int i_mask = (i_link * n_streams) + i_stream;
        uint64_t mask = m_dtp_cfg.masks.at(i_mask);
        TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting masks for link " << i_link << " stream " << i_stream << " to " << std::hex << mask;
        m_pod->get_link_processor_node(i_link).set_channel_mask_all(i_stream, mask);
      }
    }
  }
  m_pod->get_node().getClient().dispatch();

  // drop empty tps
  for (uint i = 0; i < n_links; ++i) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": dropping empty tps in pipelines " << i;
    m_pod->get_link_processor_node(i).drop_empty(true);
  }
  m_pod->get_node().getClient().dispatch();

  //
  // FIRE WIBULATORS (if required by config)
  //
  if (m_dtp_cfg.source == "int") {
    if (m_pod) {
      for (uint i = 0; i < m_pod->get_n_links(); ++i) {
        m_pod->get_wibulator_node(i).fire();
      }
    } else {
      throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
    }
  }

  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_configure() method";
}


//-----------------------------------------------------------------------------
void DTPController::do_start(const data_t& /* args */) {

  // here we will need to setup the FW config
  // ie. number of links, number of pipes etc.
  uint n_links = m_pod->get_n_links();
  uint n_streams = m_pod->get_n_streams();

  // disable links
  for (uint i = 0; i < n_links; ++i) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting up link processor " << i;
    m_pod->get_link_processor_node(i).setup_dr(false);
  }
  m_pod->get_node().getClient().dispatch();

  // pedestal capture ON
  for (uint i_link = 0; i_link < n_links; ++i_link) {
    for (uint i_stream = 0; i_stream < n_streams; ++i_stream) {
      TLOG_DEBUG(TLVL_INFO) << get_name() << ":capturing pedestal for link " << i_link << " stream " << i_stream;
      m_pod->get_link_processor_node(i_link).capture_pedestal(i_stream, true);
    }
  }
  // This is unnecessary, capture_pedestal is applied immediately (why?)
  m_pod->get_node().getClient().dispatch();


  // enable links
  for (uint i = 0; i < n_links; ++i) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting up link processor " << i;
    m_pod->get_link_processor_node(i).setup_dr(true);
  }
  m_pod->get_node().getClient().dispatch();

  // pause to ensure pedestals are all captured
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": waiting for pedestal captures";
  sleep(1);

  // pedestal capture OFF
  for (uint i_link = 0; i_link < n_links; ++i_link) {
    for (uint i_stream = 0; i_stream < n_streams; ++i_stream) {
      TLOG_DEBUG(TLVL_INFO) << get_name() << ": disabling pedestal capture for link " << i_link << " stream " << i_stream;
      m_pod->get_link_processor_node(i_link).capture_pedestal(i_stream, false);
    }
  }
  // This is unnecessary, capture_pedestal is applied immediately (why?)
  m_pod->get_node().getClient().dispatch();

  // sleep(5);

  // enable TP output
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": enabling output to DMA (backpressure disabled)";
  // TOFIX: Switch to enums!
  m_pod->get_flowmaster_node().set_outflow(1);

  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_start() method";
}


//-----------------------------------------------------------------------------
void DTPController::do_stop(const data_t& /* args */) {
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_stop() method";

  int n_links = m_pod->get_n_links();

  // disable links
  for (uint i = 0; i < n_links; ++i) {
    TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting up link processor " << i;
    m_pod->get_link_processor_node(i).setup_dr(false);
  }
  m_pod->get_node().getClient().dispatch();

  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_stop() method";
}


//-----------------------------------------------------------------------------
void DTPController::do_scrap(const data_t& /* args */) {
  // delete the pod controller
  m_pod.reset();
}


//-----------------------------------------------------------------------------
void DTPController::do_reset(const data_t& /* args */) {
  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_reset() method";

  if (m_pod) {
    m_pod->reset();
  } else {
    throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
  }

  TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_reset() method";
}

//-----------------------------------------------------------------------------
void DTPController::get_info(opmonlib::InfoCollector& ci, int /*level*/) {

  uint n_links = m_pod->get_n_links();
  uint n_streams = m_pod->get_n_streams();

  for (uint i_link = 0; i_link < n_links; ++i_link) {
    for (uint i_stream = 0; i_stream < n_streams; ++i_stream) {
      dtpcontrollerinfo::StreamInfo info;
      auto pkt_ctr = m_pod->get_link_processor_node(i_link).get_central_router_node(i_stream).getNode("csr.s3_crif-out.pkt_ctr").read();
      m_pod->get_node().getClient().dispatch();
      info.cr_if_packet_counter = pkt_ctr.value();
      ci.add(info);
    }
  }
  // auto pkt_ctr = m_pod->get_crif_node().getNode("csr.s3_crif-out.pkt_ctr").read();
  // m_pod->get_node().getClient().dispatch();

  // module_info.dummy = pkt_ctr.value();
  // ci.add(module_info);


}

}  // namespace dtpctrllibs
}  // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::dtpctrllibs::DTPController)
