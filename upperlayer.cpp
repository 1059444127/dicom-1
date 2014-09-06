#include "upperlayer.hpp"

#include <map>
#include <utility>
#include <vector>
#include <algorithm>
#include <ostream>
#include <cassert>
#include <functional>
#include <initializer_list>

#include <boost/asio.hpp>

#include "upperlayer_properties.hpp"
#include "upperlayer_statemachine.hpp"


namespace upperlayer
{
namespace
{
using uchar = unsigned char;

std::size_t be_char_to_16b(std::vector<uchar> bs)
{
   assert(bs.size() == 2);
   std::size_t sz = 0;
   sz |= (static_cast<std::size_t>(bs[0]) << 8);
   sz |= (static_cast<std::size_t>(bs[1]));
   return sz;
}
std::size_t be_char_to_32b(std::vector<uchar> bs)
{
   assert(bs.size() == 4);
   std::size_t sz = 0;
   sz |= (static_cast<std::size_t>(bs[0]) << 24);
   sz |= (static_cast<std::size_t>(bs[1]) << 16);
   sz |= (static_cast<std::size_t>(bs[2]) << 8);
   sz |= (static_cast<std::size_t>(bs[3]));
   return sz;
}

}



scx::scx(std::initializer_list<std::pair<TYPE, std::function<void(scx*, std::unique_ptr<property>)>>> l):
   statem {},
   handlers {}
{
   for (const auto p : l) {
      handlers[p.first] = p.second;
   }
}

scx::~scx()
{
}


void scx::send(property* p)
{
   auto pdu = p->make_pdu();
   auto ptype = get_type(pdu);

   if (statem.transition(ptype, false) != statemachine::CONN_STATE::INV) {
      boost::asio::async_write(sock(), boost::asio::buffer(pdu),
         [=](const boost::system::error_code& error, std::size_t bytes) { }
      );
   }
}

void scx::do_read()
{
   static std::vector<unsigned char> size(6); // watch for scope
   static std::vector<unsigned char> rem_data;
   static std::vector<unsigned char> compl_data;
   boost::asio::async_read(sock(), boost::asio::buffer(size), boost::asio::transfer_exactly(6),
      [=](const boost::system::error_code& error, std::size_t bytes)  {
         assert(bytes == 6);

         std::size_t len = be_char_to_32b({size.begin()+2, size.begin()+6});
         rem_data.resize(len);
         boost::asio::async_read(sock(), boost::asio::buffer(rem_data), boost::asio::transfer_exactly(len),
            [=](const boost::system::error_code& error, std::size_t bytes) {

               compl_data.reserve(size.size() + rem_data.size());
               compl_data.insert(compl_data.end(), size.begin(), size.end());
               compl_data.insert(compl_data.end(), rem_data.begin(), rem_data.end());
               auto pdutype = get_type(compl_data);

               //state = transition_table_received_pdus[std::make_pair(state, pdutype)];
               statem.transition(pdutype, true);

               // call appropriate handler
               handlers[pdutype](this, make_property(compl_data));

               size.clear(); rem_data.clear(); compl_data.clear();
               size.resize(6);

               // be ready for new incoming data
               if (get_state() != statemachine::CONN_STATE::STA2) {
                  do_read();
               }
            }
         );
      }
   );
}

void scx::run()
{
   io_s().run();
}



void scx::inject(TYPE t, std::function<void (scx*, std::unique_ptr<property>)> f)
{
   handlers[t] = f;
}

statemachine::CONN_STATE scx::get_state()
{
   return statem.get_state();
}


scp::scp(short port, std::initializer_list<std::pair<TYPE, std::function<void(scx*, std::unique_ptr<property>)>>> l):
   scx {l},
   io_service(),
   socket(io_service),
   acptr(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
   //acptr.accept(socket);
   acptr.async_accept(socket, [this](boost::system::error_code ec)
      {
         if (!ec) {
            do_read();
         }
      } );
}

scu::scu(std::string host, std::string port, std::initializer_list<std::pair<TYPE, std::function<void(scx*, std::unique_ptr<property>)>>> l):
   scx{l},
   io_service(),
   resolver(io_service),
   query(host, port),
   endpoint_iterator(resolver.resolve(query)),
   socket(io_service)
{
   boost::asio::ip::tcp::resolver::iterator end;
   boost::system::error_code error = boost::asio::error::host_not_found;
   while(error && endpoint_iterator != end)
   {
     socket.close();
     socket.connect(*endpoint_iterator++, error);
   }
   assert(!error);
}

boost::asio::ip::tcp::socket&scp::sock()
{
   return socket;
}

boost::asio::io_service&scp::io_s()
{
   return io_service;
}

scp::~scp()
{
   switch (get_state()) {
      case statemachine::CONN_STATE::STA2:
         break;
      default: {
         a_abort ab;
         boost::asio::write(sock(), boost::asio::buffer(ab.make_pdu()));
      }
   }
}

boost::asio::ip::tcp::socket& scu::sock()
{
   return socket;
}

boost::asio::io_service& scu::io_s()
{
   return io_service;
}

scu::~scu()
{
   switch (get_state()) {
      case statemachine::CONN_STATE::STA2:
         break;
      default: {
         a_abort ab;
         boost::asio::write(sock(), boost::asio::buffer(ab.make_pdu()));
      }
   }
}

}
