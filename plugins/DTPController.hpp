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

// appfwk
#include "appfwk/app/Nljs.hpp"
#include "appfwk/cmd/Nljs.hpp"
#include "appfwk/cmd/Structs.hpp"
#include "appfwk/DAQModule.hpp"

#include <memory>
#include <string>

namespace dunedaq {
  namespace dtpctrllibs {

    class DTPController : public dunedaq::appfwk::DAQModule
    {
    public:
      explicit DTPController(const std::string& name);

      DTPController(const DTPController&) = delete; ///< DTPController is not copy-constructible
      DTPController& operator=(const DTPController&) = delete; ///< DTPController is not copy-assignable
      DTPController(DTPController&&) = delete;            ///< DTPController is not move-constructible
      DTPController& operator=(DTPController&&) = delete; ///< DTPController is not move-assignable

      void init(const data_t& init_data) override;

    private:

      dtpcontroller::ConfParams m_dtp_configuration;

      // Commands
      void do_configure(const data_t& args);

      void do_reset(const data_t& args);

      //      void get_info(opmonlib::InfoCollector& ci, int level);

      const dtpcontrols::DTPPodNode* m_dtp_pod;

    };

  } // namespace dtpctrllibs
} // namespace dunedaq

#endif // DTPCTRLLIBS_PLUGINS_DTPCONTROLLER_HPP_
