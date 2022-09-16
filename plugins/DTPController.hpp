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
#include "dtpcontrols/DTPPodController.hpp"

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
#include <regex>

namespace dunedaq {
  namespace dtpctrllibs {

    inline std::string
    resolve_environment_variables(std::string text) {
      static const std::regex env_re{R"--(\$\{([^}]+)\})--"};
      std::smatch match;
      while (std::regex_search(text, match, env_re)) {
        auto const from = match[0];
        const char* s = getenv(match[1].str().c_str());
        const std::string env_var(s == nullptr ? "" : s);
        text.replace(from.first, from.second, env_var);
      }
      return text;
    }

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
      void do_scrap(const data_t& args);
      void do_reset(const data_t& args);

      void get_info(opmonlib::InfoCollector& ci, int level) override;

      // connections to the hardware
      std::unique_ptr<dtpcontrols::DTPPodController> m_pod;

      // config data
      dtpcontroller::Conf m_dtp_cfg;

    };

  } // namespace dtpctrllibs
} // namespace dunedaq

#endif // DTPCTRLLIBS_PLUGINS_DTPCONTROLLER_HPP_
