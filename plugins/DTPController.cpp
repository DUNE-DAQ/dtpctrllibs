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
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <string>
#include <vector>
#include <regex>

#define TLVL_INFO 10

namespace dunedaq {

  ERS_DECLARE_ISSUE(dtpctrllibs, InvalidUHALLogLevel, "Invalid UHAL log level supplied: " << log_level, ((std::string)log_level))
  ERS_DECLARE_ISSUE(dtpctrllibs, UHALConnectionsFileIssue, "Connection file not found : " << filename, ((std::string)filename))
  ERS_DECLARE_ISSUE(dtpctrllibs, UHALDeviceNameIssue, "Device name not found : " << devicename, ((std::string)devicename))
  ERS_DECLARE_ISSUE(dtpctrllibs, ModuleNotConfigured, "Cannot complete operation since " << name << "is not configured", ((std::string)name) )
  
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

    }
    
    void DTPController::init(const data_t& /* init_data */) {
      TLOG_DEBUG(TLVL_INFO) << get_name() << ": init() method does nothing";
    }
    
    void
    DTPController::do_configure(const data_t& args)
    {
      
      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_configure() method";
      
      m_dtp_cfg = args.get<dtpcontroller::ConfParams>();
      
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
      conn_file+=m_dtp_cfg.connections_file;
      resolve_environment_variables(conn_file);
      try {
	m_cm = std::make_unique<uhal::ConnectionManager>(conn_file, pcols);
      } catch (const uhal::exception::FileNotFound& excpt) {
	throw UHALConnectionsFileIssue(ERS_HERE, conn_file, excpt);
      }
      
      // get the device
      try {
	m_flx = std::make_unique<uhal::HwInterface>( m_cm->getDevice(m_dtp_cfg.device) );
      } catch (const uhal::exception::ConnectionUIDDoesNotExist& except) {
	throw UHALDeviceNameIssue(ERS_HERE, m_dtp_cfg.device, except);
      }
      
      m_dtp_pod = std::make_unique<dtpcontrols::DTPPodNode>( m_flx->getNode(std::string("")) );
      
      // here we will need to setup the FW config
      // ie. number of links, number of pipes etc.


      // reset
      m_dtp_pod->reset();
      
      // set CRIF to drop empty
      m_dtp_pod->get_crif_node().set_drop_empty();

      // set source
      if (m_dtp_cfg.source == "ext") {
	m_dtp_pod->get_flowmaster_node().set_source_gbt();
      }
      else {
	m_dtp_pod->get_flowmaster_node().set_source_wtor();
      }

      // set sink
      m_dtp_pod->get_flowmaster_node().set_sink_hits();

      // setup links
      int n_links = m_dtp_pod->get_n_links();
      int n_streams = m_dtp_pod->get_n_streams();
      for (int i = 0; i<n_links; ++i) {
	TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting up link processor " << i;
	m_dtp_pod->get_link_processor_node(i).setup(true, true, 0x7fff);
	sleep(1);
      }

      // set masks
      int masks_size = m_dtp_cfg.masks.size();
      if (masks_size == n_links * n_streams) {

	for (uint32_t i_link=0; i_link < static_cast<uint32_t>(n_links); i_link++) {
	  for (uint32_t i_stream=0; i_stream < static_cast<uint32_t>(n_streams); i_stream++) {
	    
	    int i_mask = (i_link * n_streams) + i_stream;
	    uint64_t mask = m_dtp_cfg.masks.at(i_mask);
	    m_dtp_pod->get_link_processor_node(i_link).set_channel_mask_all( i_stream, mask );
	    
	  }
	}

      }


      // reudce threshold for running
      TLOG_DEBUG(TLVL_INFO) << get_name() << ": waiting for pedestal to settle...";

      sleep(2);

      for (uint32_t i_link = 0; i_link<static_cast<uint32_t>(n_links); ++i_link) {
	TLOG_DEBUG(TLVL_INFO) << get_name() << ": setting threshold for link processor " << i_link << " to " << m_dtp_cfg.threshold;
	for (uint32_t i_stream = 0; i_stream<static_cast<uint32_t>(m_dtp_pod->get_n_streams()); ++i_stream) {
	  m_dtp_pod->get_link_processor_node(i_link).set_threshold(i_stream, m_dtp_cfg.threshold);
	}
      }

      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_configure() method";

    }
    
    void
    DTPController::do_start(const data_t& /* args */) {

      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_start() method";

      if (m_dtp_pod) {
	m_dtp_pod->enable_crif();
      }
      else {
        throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
      }

      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_start() method";
    }

    void
    DTPController::do_stop(const data_t& /* args */) {
      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_stop() method";

      if (m_dtp_pod) {
	m_dtp_pod->disable_crif();
      }
      else {
        throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
      }

      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_stop() method";
    }


    void                                                                  
    DTPController::do_reset(const data_t& /* args */)
    {
      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Entering do_reset() method";

      if (m_dtp_pod) {
	m_dtp_pod->reset();
      }
      else {
	throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
      }

      TLOG_DEBUG(TLVL_INFO) << get_name() << ": Exiting do_reset() method";
    }


    void
    DTPController::get_info(opmonlib::InfoCollector& ci, int /*level*/)
    {
      dtpcontrollerinfo::Info module_info;
      module_info.dummy = 4;
      ci.add(module_info);
    }
    
    void
    DTPController::resolve_environment_variables(std::string& input_string)
    {
      static std::regex env_var_pattern("\\$\\{([^}]+)\\}");
      std::smatch match;
      while (std::regex_search(input_string, match, env_var_pattern)) {
	const char* s = getenv(match[1].str().c_str());
	const std::string env_var(s == nullptr ? "" : s);
	input_string.replace(match[0].first, match[0].second, env_var);
      }
    }

    
  } // namespace dtpctrllibs
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::dtpctrllibs::DTPController)
