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

//#include "dtpctrllibs/DTPIssues.hpp"

// dtpcontrols headers
#include "dtpcontrols/toolbox.hpp"

#include "uhal/ConnectionManager.hpp"
#include "uhal/log/exception.hpp"
#include "uhal/utilities/files.hpp"

#include "logging/Logging.hpp"

// appfwk headers
#include "appfwk/DAQModuleHelper.hpp"
#include "appfwk/cmd/Nljs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include <string>
#include <vector>
#include <regex>

#define TLVL_ENTER_EXIT_METHODS 10

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
    
    void DTPController::init(const data_t& init_data) {
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering init() method";
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting init() method";
    }
    
    void
    DTPController::do_configure(const data_t& args)
    {
      
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_configure() method";
      
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
      
      // reset and configure
      m_dtp_pod->reset();
      //      m_dtp_pod->configure();
      
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_configure() method";

    }
    
    void
    DTPController::do_start(const data_t& args) {

      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_start() method";

      if (m_dtp_pod) {
	//	m_dtp_pod->enable();
      }
      else {
        throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
      }

      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_start() method";
    }

    void
    DTPController::do_stop(const data_t& args) {
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_stop() method";

      if (m_dtp_pod) {
	//	m_dtp_pod->disable();
      }
      else {
        throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
      }

      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_stop() method";
    }


    void                                                                  
    DTPController::do_reset(const data_t& args)
    {
      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Entering do_reset() method";

      if (m_dtp_pod) {
	m_dtp_pod->reset();
      }
      else {
	throw ModuleNotConfigured(ERS_HERE, std::string("DTPController"));
      }

      TLOG_DEBUG(TLVL_ENTER_EXIT_METHODS) << get_name() << ": Exiting do_reset() method";
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
