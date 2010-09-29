//   GreenReg framework
//
// LICENSETEXT
//
//   Copyright (C) 2008-2009 : GreenSocs Ltd
//    http://www.greensocs.com/ , email: info@greensocs.com
//
//   Developed by :
//   
//
//
//   This program is free software.
// 
//   If you have no applicable agreement with GreenSocs Ltd, this software
//   is licensed to you, and you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
// 
//   If you have a applicable agreement with GreenSocs Ltd, the terms of that
//   agreement prevail.
// 
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
// 
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//   02110-1301  USA 
// 
// ENDLICENSETEXT

#ifndef _GR_AMBASOCKET_H_
#define _GR_AMBASOCKET_H_

#include <systemc>
#include "greenreg/greenreg.h"
#include "greensocket/utils/greensocketaddress_base.h"
#include "greenreg/greenreg_socket/transactor_if.h"
#include "amba.h"
#include "greenreg/framework/core/gr_common.h"

#include <iostream>

namespace gs {
namespace amba {

class generic_slave_base {
  public:
    generic_slave_base(gs::reg::I_register_container_bus_access * _registers, gr_uint_t _base_addr, gr_uint_t _decode_size)
    : m_registers( _registers), m_decode_size( _decode_size), m_base_addr( _base_addr), m_delay_enabled(true) // enabled by default
    {}

    virtual ~generic_slave_base() {}

    inline gr_uint_t get_decode_size() { 
      return m_decode_size;
    }
    
    /// Disables the delay for all notification rule callbacks caused by this socket
    void disable_delay() { 
      m_delay_enabled = false;
    }
    
    /// Enables the delay for all notification rule callbacks caused by this socket
    void enable_delay() { 
      m_delay_enabled = true;
    }
    
    /// Returns if the delay is enabled
    bool delay_enabled() {
      return m_delay_enabled;
    }
    
  protected:
    bool write_to_module( unsigned int _data, unsigned int _address, unsigned int _byte_enable, gs::reg::transaction_type* _transaction, bool _delayed) {
      return( m_registers->bus_write( _data, _address, _byte_enable, _transaction, _delayed));
    }

    unsigned int read_from_module( unsigned int& _data, unsigned int _address, unsigned int _byte_enable, gs::reg::transaction_type* _transaction, bool _delayed) {
      return( m_registers->bus_read( _data, _address, _byte_enable, _transaction, _delayed));
    }

    gs::reg::I_register_container_bus_access * m_registers;
    gr_uint_t m_decode_size;

  protected:
    gr_uint_t m_base_addr;
    
    /// If this socket let the caused notification rule callbacks be delayed
    bool m_delay_enabled;
    
  };

template<unsigned int BUSWIDTH = 32>
class amba_slave : public generic_slave_base
                 , public ::amba_slave_base
                 , public ::amba::amba_slave_socket<BUSWIDTH>
                 , public gs::socket::GreenSocketAddress_base {
  public:

    amba_slave( sc_core::sc_module_name _name, gs::reg::I_register_container_bus_access & _reg_bind, 
                gr_uint_t _base_address, gr_uint_t _decode_size, ::amba::amba_bus_type _type, ::amba::amba_layer_ids _layer, bool _arbiter)
      : generic_slave_base( &_reg_bind, _base_address, _decode_size)
      , ::amba::amba_slave_socket<BUSWIDTH>(_name, _type, _layer, _arbiter)
      , ::gs::socket::GreenSocketAddress_base((const char *)_name)
      , m_base( _base_address)
      , m_high( _base_address + _decode_size) {

      // Bind amba blocking ...
      amba_slave::register_b_transport(this, &amba_slave::b_transport);

      gs::socket::GreenSocketAddress_base::base_addr = _base_address;
      gs::socket::GreenSocketAddress_base::high_addr = _base_address + _decode_size;
        
    }

    virtual ~amba_slave() {}

  public:
    void b_transport(tlm::tlm_generic_payload& gp, sc_core::sc_time&) {
      unsigned int address = gp.get_address() - m_base;
      unsigned int length;
      unsigned int mask;
      unsigned int data;

      gs::reg::transaction_type trans(this, &gp);

      if (gp.is_write()) {
        //for (unsigned int i=0; i<gp.get_data_length(); i+=4) {  // Works only in nonburst mode max 4byte atime
          length = gp.get_data_length();
          switch(length) {
            case  1: mask = 0x1; break;
            case  2: mask = 0x3; break;
            case  3: mask = 0x7; break;
            default: mask = 0xF; break;
          }

          memcpy(&data, &(gp.get_data_ptr()[0]), length);

        //  std::cout<<"    "<<address<<": "<<std::hex<<"0x"<<((gp.get_data_ptr()[address]<16)?"0":"")<<((unsigned short)gp.get_data_ptr()[address])<<std::endl;
          m_registers->bus_write( data, address, mask, &trans, m_delay_enabled);
        //}
      } 
      if (gp.is_read()){
        //for(unsigned int i=0; i<gp.get_data_length(); i+=4){
          switch(gp.get_data_length()) {
            case  1: mask = 0x1; break;
            case  2: mask = 0x3; break;
            case  3: mask = 0x7; break;
            default: mask = 0xF; break;
          }
          //mask = (0xFFFFFFFF)>>(32-(address&0x3));
          //gp.get_data_ptr()[address]=rddata++;
      //    std::cout<<"    "<<address<<": "<<std::hex<<"0x"<<((gp.get_data_ptr()[address]<16)?"0":"")<<((unsigned short)gp.get_data_ptr()[address])<<std::endl;
          m_registers->bus_read( m_bus_read_data, address, mask, &trans, m_delay_enabled);
          gs::MData mdata(gs::GSDataType::dtype((unsigned char *)&m_bus_read_data, trans->getMBurstLength()));  
          trans->setSData(mdata);
        //}
      }
      gp.set_response_status(tlm::TLM_OK_RESPONSE);
    }

    // tlm_slave_if implementation
    virtual void setAddress(sc_dt::uint64 base, sc_dt::uint64 high) {
      m_base = base;
      m_high = high;

      gs::socket::GreenSocketAddress_base::base_addr = base;
      gs::socket::GreenSocketAddress_base::high_addr = high;
    }
    
    sc_dt::uint64 get_base_addr() {
      return m_base;
    }

    sc_dt::uint64 get_size() {
      return m_high - m_base;  
    }

    virtual sc_core::sc_object *get_parent() {
      std::cout << "my get_parent" << std::endl;
      return this;
    }

    inline void set_delay() {}
    
  protected:
    sc_dt::uint64  m_base;
    sc_dt::uint64  m_high;
    
  private:
    unsigned int m_bus_read_data;
};

template<unsigned int BUSWIDTH = 32>
class amba_master : public tlm_components::transactor_if
                  , public ::amba::amba_master_socket<BUSWIDTH> {
//                  , public sc_core::sc_module {
  public:
    typedef ::amba::amba_master_socket<BUSWIDTH> socket;

    amba_master(sc_core::sc_module_name _name, ::amba::amba_bus_type _type, ::amba::amba_layer_ids _layer, bool _arbiter) : socket(_name, _type, _layer, _arbiter) {}

    virtual ~amba_master() {}
    virtual void random_read() {}
    virtual void random_write() {}

  protected:
     typedef tlm::tlm_generic_payload payload_t;

    // transactor methods
    virtual void _read(unsigned _address, unsigned _length, unsigned int* _db, bool _bytes_enabled, unsigned int* _be) {
      // fix because greenbus does not overwrite data block cleanly (even though the back end does)
      // Payload immer vom socket hohlen damit der interne pool benutzt wird, ist schneller und sauberer.
      payload_t *gp = socket::get_transaction();
      *_db = 0;
      sc_core::sc_time t;
      gp->set_command(tlm::TLM_READ_COMMAND);
      gp->set_address(_address);
      gp->set_data_length(_length);
      gp->set_streaming_width(4);
      gp->set_byte_enable_ptr(NULL);
      gp->set_data_ptr(reinterpret_cast<unsigned char *>(_db));
      socket::m_if_wrapper.b_transport(*gp, t);
      //wait(t); // LT -> always 0
      socket::release_transaction(gp);
    }

    virtual void _write(unsigned _address, unsigned _length, unsigned int* _db, bool _bytes_enabled, unsigned int* _be) {
      sc_core::sc_time t;
      payload_t *gp = socket::get_transaction();
      gp->set_command(tlm::TLM_WRITE_COMMAND);
      gp->set_address(_address);
      gp->set_data_length(_length);
      gp->set_streaming_width(4);
      gp->set_byte_enable_ptr(NULL);
      gp->set_data_ptr(reinterpret_cast<unsigned char *>(_db));
      socket::m_if_wrapper.b_transport(*gp, t);
      //wait(t); // LT -> always 0
      socket::release_transaction(gp);
    }

  private:
  };

} // end namespace amba
} // end namespace gs

#endif /*_GR_AMBASOCKET_H_*/