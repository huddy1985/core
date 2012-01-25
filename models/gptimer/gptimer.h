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
// Title:      gptimer.h
//
// ScssId:
//
// Origin:     HW-SW SystemC Co-Simulation SoC Validation Platform
//
// Purpose:    header file containing the definition of the gptimer
//             model. Due to the fact that the gptimer class is a
//             template class it includes its implementation from
//             gptimer.tpp
//
// Modified on $Date$
//          at $Revision$
//          by $Author$
//
// Principal:  European Space Agency
// Author:     VLSI working group @ IDA @ TUBS
// Maintainer: Rolf Meyer
// Reviewed:
//*********************************************************************

#ifndef GPTIMER_H
#define GPTIMER_H

#include <greenreg_ambasockets.h>
#include <greencontrol/all.h>
#include <systemc>
#include <boost/config.hpp>

#include "signalkit.h"
#include "power_monitor.h"
#include "verbose.h"

#include "gpcounter.h"
#include "apbdevice.h"
#include "clkdevice.h"

#include <string>
#include <ostream>
#include <vector>

/// @addtogroup gptimer GPTimer
/// @{

/// @brief This class is a TLM 2.0 Model of the Aeroflex Gaisler GRLIB GPTimer.
/// Further informations to the original VHDL Modle are available in the GRLIB IP Core User's Manual Section 37
class GPTimer : public gs::reg::gr_device, public APBDevice, public CLKDevice {
    public:
        SC_HAS_PROCESS(GPTimer);
        SK_HAS_SIGNALS(GPTimer);
        GC_HAS_CALLBACKS();
        /// APB Slave socket for all bus communication
        gs::reg::greenreg_socket<gs::amba::amba_slave<32> > bus;

        signal<bool>::selector irq;
        signal<bool>::out wdog;

        /// Stores the default config register value
        unsigned int conf_defaults;

        /// Stores the time when the value of the prescaler was known.
        sc_core::sc_time lasttime;

        /// Stores the last known prescaler value.
        ///
        /// A prescaler value is known when it gets set by the prescaler value register
        /// or when its get calculatet befor a complete stop.
        ///
        /// It gets always set with the lasttime. Both define the prescaler function
        /// implemented in valueof.
        unsigned int lastvalue;

        /// The tick event.
        ///
        /// This event gets set to calculate and produce the tick value for the prescaler underflow.
        /// The value gets calculated either when the prescaler reset or value register change or when the undeflow just happend.
        /// See tick_calc for calculation and ticking for the event wait statement.
        ///
        sc_core::sc_event e_tick;

        /// A vector of Counter classes, each representate an internal counter.
        // TODO Replace by Array instanciated at construction time.
        std::vector<GPCounter *> counter;


        /// Creates an instance of an GPTimer.
        ///
        /// @param name    The name of the instance. It's needed for debunging.
        /// @param ncounters Defines the number of counters in the unit. Default is 1. Max is 7.
        /// @param gpindex TODO
        /// @param gpaddr  TODO
        /// @param gpmask  TODO
        /// @param gpirq   Defines which APB interupt the timers will generate. Default is 0.
        /// @param gsepirq If set to 1, each timer will drive an individual interrupt line,
        ///                starting with interrupt irq. If set to 0, all timers will drive
        ///                the same interrupt line (irq).
        /// @param pindex  APB Bus Index. Defines the slave index at the APB Bus.
        /// @param ntimers Defines the number of timers in the unit. Default is 1. Max is 7.
        /// @param gnbits  Defines the number of bits in the timers. Default is 32.
        /// @param gsbits  Defines the number of bits in the scaler. Default is 16.
        /// @param gwdog   Watchdog reset value. When set to a non-zero value, the
        ///                last timer will be enabled and pre-loaded with this value
        ///                at reset. When the timer value reaches 0, the WDOG output
        ///                is driven active.
        GPTimer(sc_core::sc_module_name name, unsigned int ncounters = 1,
                 int pindex = 0, int paddr = 0, int pmask = 4095, int pirq = 0, 
                 int sepirq = 0, int sbits = 16, int nbits = 32, int wdog = 0,
                 bool powmon = false);

        /// Free all counter and unregister all callbacks.
        ~GPTimer();

        /// Execute the callback registering when systemc reaches the end of elaboration.
        void end_of_elaboration();

        
      // Register Callbacks
        /// Register callback executed before the prescaler value register gets read, to calculate the current value.
        void scaler_read();

        /// Register callback executed after the presacler value is written, to recalculate the counter and ticking functions.
        void scaler_write();

        /// Register callback executed after the presacler reset is written, to recalculate the counter and ticking functions.
        void screload_write();

        /// Register callback executed before the prescaler config register gets read, to calculate the current value.
        void conf_read();

      // Signal Callbacks
        /// Signal callback executed a reset on the timer when the rst signal arrives.
        /// 
        /// @param value The new Value of the Signal.
        /// @param time A possible delay. Which means the reset might be performed in the future (Not used for resets!).
        virtual void dorst();

      // Threads
        /// Diagnostic Thread
        ///
        ///  diag is a diagnostic SC_THREAD it gets triggert once in a clockcycle. But this module does not receive
        ///  a clock it calculates the clockcycle from the values set by a clk function.
        ///  In this function are debuging commands to read current signals or register values.
        ///
        void diag();

      // Functions
        /// The time to value function of the prescaler or the counters.
        ///
        ///  This is the fundamental function which defines the connection between a given time and the value.
        ///  All calculations in the Timer and Counter class are based on this function.
        ///  The function itself is defined through the two attributes lastvalue and lasttime.
        ///
        /// @param t         The Time for which is needed the prescaler value.
        /// @param offset    An offset, defined to calculate a value which is influenced by the counter number.
        ///                  This is needed due to the fact that the rtl model decrements the counters one after one,
        ///                  Each in a clockcycle if the prescaler undeflows.
        /// @param cycletime The cycle length which decrements the value.
        /// @return          The value to time t.
        ///
        /// @see lastvalue
        /// @see lasttime
        /// @see numberofticksbetween()
        ///
        int32_t valueof(sc_core::sc_time t, int32_t offset, sc_core::sc_time cycletime) const;

        /// @brief TODO
        ///
        int numberofticksbetween(sc_core::sc_time a, sc_core::sc_time b, int counter, sc_core::sc_time cycletime);
       
        const uint32_t g_sbits;
        const uint32_t g_nbits;
        const uint32_t g_wdog_length;
        const uint32_t powermon;
      // Register Value Offsets, Masks and Bits
        /// Scaler Value Register Address
        static const uint32_t SCALER = 0x00;
        
        /// Scaler Relaod Register Address
        static const uint32_t SCRELOAD = 0x04;
        
        /// Scaler Configuration Register Address
        static const uint32_t CONF = 0x08;

        /// Returns Counter Value Register Address for Counter nr
        ///
        /// @param nr Counter Number
        static const uint32_t VALUE(uint8_t nr) {
            return 0x10 * (nr + 1) + 0x0;
        }

        /// Returns Counter Reload Register Address for Counter nr
        ///
        /// @param nr Counter Number
        static const uint32_t RELOAD(uint8_t nr) {
            return 0x10 * (nr + 1) + 0x4;
        }

        /// Returns Counte Control Register Address for Counter nr
        ///
        /// @param nr Counter Number
        static const uint32_t CTRL(uint8_t nr) {
            return 0x10 * (nr + 1) + 0x8;
        }

        /// Position of the Bit in the Scaler Configuration Register
        static const uint32_t CONF_DF = 9;
        
        /// Position of the SetInterrupt Bit in the Scaler Configuration Register
        static const uint32_t CONF_SI = 8;
        
        /// Mask of the IRQ Bits in the Scaler Configuration Register
        static const uint32_t CONF_IQ_MA = 0x000000F8;
        
        /// Offset of the IRQ Bits in the Scaler Configuration Register
        static const uint32_t CONF_IQ_OS = 3;
        
        /// Mask of the Counter Number Bits in the Scaler Configuration Register
        static const uint32_t CONF_NR_MA = 0x00000007;
        
        /// Offset of the Counter Number Bits in the Scaler Configuration Register
        static const uint32_t CONF_NR_OS = 0;

        /// Position of the Debug Halt Bit in the Counter Control Registers
        static const uint32_t CTRL_DH = 6;

        /// Position of the Chaining Bit in the Counter Control Registers
        static const uint32_t CTRL_CH = 5;

        /// Position of the Interrupt Pending Bit in the Counter Control Registers
        static const uint32_t CTRL_IP = 4;

        /// Position of the Interrupt Enable Bit in the Counter Control Registers
        static const uint32_t CTRL_IE = 3;

        /// Position of the Load Bit in the Counter Control Registers
        static const uint32_t CTRL_LD = 2;

        /// Position of the Reset Bit in the Counter Control Registers
        static const uint32_t CTRL_RS = 1;

        /// Position of the Enable Bit in the Counter Control Registers
        static const uint32_t CTRL_EN = 0;

        friend class GPCounter;
};

/// @}

#endif // GPTIMER_H
