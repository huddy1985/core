//*********************************************************************
// Copyright 2010, Institute of Computer and Network Engineering,
//                 TU-Braunschweig
// All rights reserved
// Any reproduction, use, distribution or disclosure of this program,
// without the express, prior written consent of the authors is 
// strictly prohibited.
//
// University of Technology Braunschweig
// Institute of Computer and Network Engineering
// Hans-Sommer-Str. 66
// 38118 Braunschweig, Germany
//
// ESA SPECIAL LICENSE
//
// This program may be freely used, copied, modified, and redistributed
// by the European Space Agency for the Agency's own requirements.
//
// The program is provided "as is", there is no warranty that
// the program is correct or suitable for any purpose,
// neither implicit nor explicit. The program and the information in it
// contained do not necessarily reflect the policy of the 
// European Space Agency or of TU-Braunschweig.
//*********************************************************************
// Title:      localram.h
//
// ScssId:
//
// Origin:     HW-SW SystemC Co-Simulation SoC Validation Platform
//
// Purpose:    Class definition of scratchpad/localram.
//             Can be attached to the icache and dcache controllers.
//             The localram enables fast 0-waitstate access
//             to instructions or data.
//
// Modified on $Date$
//          at $Revision$
//          by $Author$
//
// Principal:  European Space Agency
// Author:     VLSI working group @ IDA @ TUBS
// Maintainer: Thomas Schuster
// Reviewed:
//*********************************************************************

#ifndef __LOCALRAM_H__
#define __LOCALRAM_H__

#include <stdint.h>
#include <greencontrol/config.h>
#include <tlm.h>

#include "mem_if.h"
#include "defines.h"

// Local scratchpad ram can optionally be attached to both instruction and data cache controllers.
// The scratch pad ram provides fast 0-waitstates ram memories for instructions and data.
// The ram can be between 1 - 512 kbyte, and mapped to any 16 Mbyte block in the address space.
// Accesses the the scratchpad ram are not cached, and will not appear on the AHB bus.
// The scratch pads do not appear on the AHB bus and can only be read or written by the
// processor. The instruction scratchpad must be initialized by software (through store instr.)
// before it can be used. The default address for the instruction ram is 0x83000000,
// and for the data ram 0x8f000000.
// ! Local scratch pad ram can only be enabled when the MMU is disabled !
// ! Address decoding and checking is done in class mmu_cache !

/// @brief Local Scratchpad RAM
class localram : public sc_core::sc_module, public mem_if {

 public:

  // Memory interface functions (mem_if):
  // -----------------------------
  /// Read from scratchpad
  virtual bool mem_read(unsigned int address, unsigned int asi, unsigned char *data, unsigned int len,
                        sc_core::sc_time *t, unsigned int *debug, bool is_dbg);
  /// Write to scratchpad
  virtual void mem_write(unsigned int address, unsigned int asi, unsigned char *data, unsigned int len,
                         sc_core::sc_time *t, unsigned int *debug, bool is_dbg);
  
  /// Helper functions for definition of clock cycle
  void clkcng(sc_core::sc_time &clk);

  /// Hook up for showing statistics
  void end_of_simulation();

  // Constructor
  // -----------
  /// @brief Constructor of scratchpad RAM implementation (localram)
  /// @param name    SystemC module name
  /// @param lrsize  Local ram size. Size in kbyte = 2^lrsize (like top-level template)
  /// @param lrstart Local ram start address. The 8 most significant bits of the address.
  localram(sc_core::sc_module_name name, 
           unsigned int lrsize, 
           unsigned int lrstart,
           bool pow_mon = false);
  
  /// Destructor
  ~localram();

  // Pointer to actual memory
  t_cache_data * scratchpad;

  // Helpers
  // -------
  t_cache_data m_default_entry;

  // Local RAM parameters
  // --------------------
  /// Size of the local ram in words
  unsigned int m_lrsize;

  /// Start address of the local ram
  unsigned int m_lrstart;

  /// Power monitoring enabled or not
  bool m_pow_mon;

  /// *****************************************************
  /// Performance Counters

  /// GreenControl API container
  gs::cnf::cnf_api *m_api;
        
  /// Open a namespace for performance counting in the greencontrol realm
  gs::gs_param_array m_performance_counters;
  
  /// Number of read accesses
  gs::gs_param<unsigned long long> sreads;

  /// Number of write accesses
  gs::gs_param<unsigned long long> swrites;

  /// Volume of total reads (bytes)
  gs::gs_param<unsigned long long> sreads_byte;

  /// Volume of total writes (bytes)
  gs::gs_param<unsigned long long> swrites_byte;

  /// *****************************************************
  /// Power Modeling Parameters

  /// Normalized static power input
  gs::gs_param<double> sta_power_norm;

  /// Normalized dynamic power input (activation independent)
  gs::gs_param<double> dyn_power_norm;

  /// Normalized read access energy
  gs::gs_param<double> dyn_read_energy_norm;

  /// Normalized write access energy
  gs::gs_param<double> dyn_write_energy_norm;

  /// Parameter array for power data output
  gs::gs_param_array power;

  /// Static power of module
  gs::gs_param<double> sta_power;

  /// Dynamic power of module (activation independent)
  gs::gs_param<double> dyn_power;

  /// Dynamic energy per read access
  gs::gs_param<double> dyn_read_energy;

  /// Dynamic energy per write access
  gs::gs_param<double> dyn_write_energy;

  /// Number of reads from memory (read & reset by monitor)
  gs::gs_param<unsigned long long> dyn_reads;

  /// Number of writes to memory (read & reset by monitor)
  gs::gs_param<unsigned long long> dyn_writes;
  
  /// Clock cycle time
  sc_core::sc_time clockcycle;

};

#endif // __LOCALRAM_H__
