/**
 * @file DTPWrapper.hpp wrapper around DTPPodNode, not realy necessary
 *
 * This is part of the DUNE DAQ , copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef DTPLIBS_SRC_DTPWRAPPER_HPP_
#define DTPLIBS_SRC_DTPWRAPPER_HPP_

#include "dtpcontrols/DTPPodNode.hpp"

#include <nlohmann/json.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace dunedaq::dtplibs {

class DTPWrapper
{
public:
  /**
   * @brief DTPWrapper Constructor
   * @param name Instance name for this DTPWrapper instance
   */
  DTPWrapper();
  ~DTPWrapper();
  DTPWrapper(const DTPWrapper&) = delete;            ///< DTPWrapper is not copy-constructible
  DTPWrapper& operator=(const DTPWrapper&) = delete; ///< DTPWrapper is not copy-assignable
  DTPWrapper(DTPWrapper&&) = delete;                 ///< DTPWrapper is not move-constructible
  DTPWrapper& operator=(DTPWrapper&&) = delete;      ///< DTPWrapper is not move-assignable

  using data_t = nlohmann::json;
  void init(const data_t& args);
  void reset();
  void configure(const data_t& args);

private:
  // Types

  // Constants

  // hardware access
  const dtpcontrols::DTPPodNode* m_dtp_pod_node;

};

} // namespace dunedaq::dtplibs

#endif // DTPLIBS_SRC_DTPWRAPPER_HPP_
