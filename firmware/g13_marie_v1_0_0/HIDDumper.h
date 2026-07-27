/* USB EHCI Host for Teensy 3.6
   Copyright 2017 Paul Stoffregen (paul@pjrc.com)

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, license, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef __HIDDumper_h_
#define __HIDDumper_h_
#include <Arduino.h>
#include <USBHost_t36.h>

class HIDDumpController : public USBHIDInput {
public:
  HIDDumpController(USBHost &host, uint32_t index = 0, uint32_t usage = 0) : index_(index), fixed_usage_(usage) { init(); }
  uint32_t usage(void) {return usage_;}
  static bool show_raw_data;
  static bool show_formated_data;
  static bool changed_data_only;
  
protected:
  virtual hidclaim_t claim_collection(USBHIDParser *driver, Device_t *dev, uint32_t topusage);
  virtual bool hid_process_in_data(const Transfer_t *transfer);
  virtual bool hid_process_out_data(const Transfer_t *transfer);
  virtual void hid_input_begin(uint32_t topusage, uint32_t type, int lgmin, int lgmax);
  virtual void hid_input_data(uint32_t usage, int32_t value);
  virtual void hid_input_end();
  virtual void disconnect_collection(Device_t *dev);
private:
  void init();

  void dumpHIDReportDescriptor(USBHIDParser *phidp);
  void printUsageInfo(uint8_t usage_page, uint16_t usage);
  void print_input_output_feature_bits(uint8_t val);

  USBHIDParser *driver_;
  uint8_t collections_claimed = 0;
  volatile int hid_input_begin_level_ = 0;
  uint32_t index_;
  uint32_t fixed_usage_;
  
  uint32_t usage_ = 0;
  // Track changing fields. 
   const static int MAX_CHANGE_TRACKED = 512;
  uint32_t usages_[MAX_CHANGE_TRACKED];
  int32_t values_[MAX_CHANGE_TRACKED];
  int count_usages_ = 0;
  int index_usages_ = 0;
  // experiment to see if we can receive data from Feature reports.
  enum {MAX_FEATURE_REPORTS=20};
  uint8_t feature_report_ids_[MAX_FEATURE_REPORTS];
  uint8_t cnt_feature_reports_ = 0;

  // See if we can contribute transfers
  Transfer_t mytransfers[2] __attribute__ ((aligned(32)));
};
#endif // __HIDDumper_h_
