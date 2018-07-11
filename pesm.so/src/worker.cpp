/********************************************************************************
 *
 * Copyright (c) 2018 ROCm Developer Tools
 *
 * MIT LICENSE:
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#include "worker.h"

#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif
#include <pci/pci.h>
#include <linux/pci.h>
#ifdef __cplusplus
}
#endif

#include "rvsliblogger.h"
#include "rvs_module.h"
#include "pci_caps.h"
#include "gpu_util.h"
#include "rvsloglp.h"

using std::string;
using std::vector;
using std::map;

Worker::Worker() {
  bfiltergpu = false;
}
Worker::~Worker() {}

/**
 * @brief Sets GPU IDs for filtering
 * @arg GpuIds Array of GPU GpuIds
 */
void Worker::set_gpuids(const std::vector<int>& GpuIds) {
  gpuids = GpuIds;
  bfiltergpu = true;
}

/**
 * @brief Thread function
 *
 * Loops while brun == TRUE and performs polled monitoring avery 1msec.
 *
 * */
void Worker::run() {
  brun = true;
  char buff[1024];

  map<string, string>::iterator it;
  vector<uint16_t> gpus_location_id;
  map<uint16_t, string> old_val;
  map<uint16_t, string> old_pwr_val;

  struct pci_access *pacc;
  struct pci_dev *dev;

  unsigned int sec;
  unsigned int usec;
  void* r;

  // get timestamp
  rvs::lp::get_ticks(sec, usec);

  // add string output
  string msg("[" + action_name + "] pesm " + strgpuids + " started");
  rvs::lp::Log(msg, rvs::logresults, sec, usec);

  // add JSON output
  r = rvs::lp::LogRecordCreate("pesm", action_name.c_str(), rvs::logresults,
                               sec, usec);
  rvs::lp::AddString(r, "msg", "started");
  rvs::lp::AddString(r, "device", strgpuids);
  rvs::lp::LogRecordFlush(r);

  // worker thread has started
  while (brun) {
    rvs::lp::Log("[" + action_name + "] pesm worker thread is running...",
                 rvs::logtrace);

    // get all GPU location_id (Note: we're not using device_id as the unique
    // identifier of the GPU because multiple GPUs can have the same ID ...
    // This is also true for the case of the machine where we're working)
    // therefore, what we're using is the location_id which is unique
    // and points to the sysfs
    gpu_get_all_location_id(gpus_location_id);

    // get the pci_access structure
    pacc = pci_alloc();
    // initialize the PCI library
    pci_init(pacc);
    // get the list of devices
    pci_scan_bus(pacc);

    // iterate over devices
    for (dev = pacc->devices; dev; dev = dev->next) {
      pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS
      | PCI_FILL_EXT_CAPS | PCI_FILL_CAPS
      | PCI_FILL_PHYS_SLOT);  // fil in the info

      // computes the actual dev's location_id (sysfs entry)
      uint16_t dev_location_id =
      ((((uint16_t)(dev->bus)) << 8) | (dev->func));

      // check if this pci_dev corresponds to one of AMD GPUs
      auto it_gpu = find(gpus_location_id.begin(), gpus_location_id.end(),
                         dev_location_id);
      if (it_gpu == gpus_location_id.end())
        continue;

      // device_id filtering
      if ( device_id != 0 && dev->device_id != device_id)
        continue;

      // gpu id filtering
      if (bfiltergpu) {
        auto itgpuid = find(gpuids.begin(), gpuids.end(), dev_location_id);
        if (itgpuid == gpuids.end())
          continue;
      }

      rvs::lp::get_ticks(sec, usec);

      // get current speed for the link
      get_link_stat_cur_speed(dev, buff);
      string new_val(buff);

      // get current power state for GPU
      get_pwr_curr_state(dev, buff);
      string new_pwr_val(buff);

      // link speed changed?
      if (old_val[dev_location_id] != new_val) {
        // new value is different, so store it;
        old_val[dev_location_id] = new_val;

        string msg("[" + action_name + "] " + "pesm "
          + std::to_string(dev_location_id) + " link speed change " + new_val);
        rvs::lp::Log(msg, rvs::loginfo, sec, usec);

        r = rvs::lp::LogRecordCreate("pesm ", action_name.c_str(), rvs::loginfo,
                                    sec, usec);
        rvs::lp::AddString(r, "msg", "link speed change");
        rvs::lp::AddString(r, "val", new_val);
        rvs::lp::LogRecordFlush(r);
      }

      // power state changed
      if (old_pwr_val[dev_location_id] != new_pwr_val) {
        // new value is different, so store it;
        old_pwr_val[dev_location_id] = new_pwr_val;

        string msg("[" + action_name + "] " + "pesm "
          + std::to_string(dev_location_id) +
          " power state change " + new_pwr_val);
        rvs::lp::Log(msg, rvs::loginfo, sec, usec);

        r = rvs::lp::LogRecordCreate("pesm", action_name.c_str(), rvs::loginfo,
                                    sec, usec);
        rvs::lp::AddString(r, "msg", "power state change");
        rvs::lp::AddString(r, "val", new_pwr_val);
        rvs::lp::LogRecordFlush(r);
      }
    }

    pci_cleanup(pacc);

    sleep(1);
  }

  // get timestamp
  rvs::lp::get_ticks(sec, usec);

  // add string output
  msg = "[" + stop_action_name + "] pesm all stopped";
  rvs::lp::Log(msg, rvs::logresults, sec, usec);

  // add JSON output
  r = rvs::lp::LogRecordCreate("PESM",
                               stop_action_name.c_str(), rvs::logresults,
                               sec, usec);
  rvs::lp::AddString(r, "msg", "stopped");
  rvs::lp::LogRecordFlush(r);

  rvs::lp::Log("[" + stop_action_name + "] pesm worker thread has finished",
               rvs::logdebug);
}

/**
 * @brief Stops monitoring
 *
 * Sets brun member to FALSE thus signaling end of monitoring.
 * Then it waits for std::thread to exit before returning.
 *
 * */
void Worker::stop() {
  rvs::lp::Log("[" + stop_action_name + "] pesm in Worker::stop()",
               rvs::logtrace);
  // reset "run" flag
  brun = false;
  // (give thread chance to finish processing and exit)
  sleep(200);

  // wait a bit to make sure thread has exited
  try {
    if (t.joinable())
      t.join();
  }
  catch(...) {
  }
}
