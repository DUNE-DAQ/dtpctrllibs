/**
 * @file DTPController.hpp DUNE TP controller DAQ Module
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef DTPCTRLLIBS_PLUGINS_DTPCONTROLLER_HPP_
#define DTPCTRLLIBS_PLUGINS_DTPCONTROLLER_HPP_

// schema
#include "dtpctrllibs/dtpcontroller/Nljs.hpp"
#include "dtpctrllibs/dtpcontroller/Structs.hpp"

// dtpcontrols
#include "dtpcontrols/DTPPodNode.hpp"

// uhal
#include "uhal/ConnectionManager.hpp"
#include "uhal/log/exception.hpp"

// appfwk
#include "appfwk/DAQModule.hpp"
#include "utilities/WorkerThread.hpp"

#include "rcif/cmd/Nljs.hpp"
#include "rcif/cmd/Structs.hpp"

#include "ers/Issue.hpp"
#include "logging/Logging.hpp"

#include "opmonlib/InfoCollector.hpp"

#include <memory>
#include <string>

namespace dunedaq {
  namespace dtpctrllibs {

    class DTPController : public dunedaq::appfwk::DAQModule
    {
    public:
      explicit DTPController(const std::string& name);
      ~DTPController();

      DTPController(const DTPController&) = delete; ///< DTPController is not copy-constructible
      DTPController& operator=(const DTPController&) = delete; ///< DTPController is not copy-assignable
      DTPController(DTPController&&) = delete;            ///< DTPController is not move-constructible
      DTPController& operator=(DTPController&&) = delete; ///< DTPController is not move-assignable

      void init(const data_t& init_data) override;

    private:

      // Commands

      void do_configure(const data_t& args);
      void do_start(const data_t& args);
      void do_stop(const data_t& args);

      void do_reset(const data_t& args);

      void get_info(opmonlib::InfoCollector& ci, int level) override;


      // helpers
      void resolve_environment_variables(std::string& input_string);

      // connections to the hardware
      std::unique_ptr<uhal::ConnectionManager> m_cm;
      std::unique_ptr<uhal::HwInterface> m_flx;
      std::unique_ptr<dtpcontrols::DTPPodNode> m_dtp_pod;

      // config data
      dtpcontroller::ConfParams m_dtp_cfg;

      
      //      std::string m_queue; // dummy queue
      //      std::unique_ptr<sink_t> m_queue;

    };

  } // namespace dtpctrllibs
} // namespace dunedaq

#endif // DTPCTRLLIBS_PLUGINS_DTPCONTROLLER_HPP_
