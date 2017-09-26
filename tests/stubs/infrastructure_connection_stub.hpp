#ifndef INFRASTRUCTURE_CONNECTION_STUB_HPP
#define INFRASTRUCTURE_CONNECTION_STUB_HPP

#include <iostream>
#include <fstream>

#include "../source/infrastructure/asio_tcp_connection.hpp"
#include "../source/infrastructure/asio_tcp_connection_manager.hpp"


/**
 * @brief The infrastrustructure_timer_stub does nothing
 */
class infrastrustructure_timer_stub : public Iinfrastructure_timeout_connection
{
      // Iinfrastructure_timeout_connection interface
   public:
      void cancel() override
      {
      }
      void wait_async() override
      {
      }
};


/**
 * @brief The infrastructure_read_connection_stub reads predefined data and
 *        returns it as canned response -> writing to it does not do anything.
 */
class infrastructure_read_connection_stub : public Iinfrastructure_upperlayer_connection
{
   private:
      std::ifstream in;
      boost::system::error_code ec;

   public:
      infrastructure_read_connection_stub(std::string binfile):
         in {binfile, std::ios::binary},
         ec {}
      {

      }

      // Iinfrastructure_upperlayer_connection interface
      void write_data(std::shared_ptr<std::vector<unsigned char>> buffer,
                      std::function<void(const boost::system::error_code&, std::size_t)> on_complete) override
      {
         on_complete(ec, buffer->size());
      }
      void write_data(void*, std::size_t len,
                      std::function<void (const boost::system::error_code&, std::size_t)> on_complete) override
      {
         on_complete(ec, len);
      }

      void read_data(std::shared_ptr<std::vector<unsigned char>> buffer,
                     std::size_t len, std::function<void(const boost::system::error_code&, std::size_t)> on_complete) override
      {
         std::istreambuf_iterator<char> instream {in};
         std::copy_n(instream, len, std::begin(*buffer));
         std::advance(instream, 1);
         on_complete(ec, len);
      }

      void read_data(std::shared_ptr<std::vector<unsigned char>> buffer,
                     std::function<void (const boost::system::error_code&, std::size_t)> on_complete) override
      {
         auto size = buffer->size();
         std::istreambuf_iterator<char> instream {in};
         //std::copy(instream, std::istreambuf_iterator<char>(), std::begin(*buffer));
         std::copy_n(instream, size, std::begin(*buffer));
         std::advance(instream, 1);
         on_complete(ec, size);
      }

      std::unique_ptr<Iinfrastructure_timeout_connection> timeout_timer(std::chrono::duration<int>, std::function<void ()>) override
      {
         return std::unique_ptr<Iinfrastructure_timeout_connection> {new infrastrustructure_timer_stub{}};
      }

      bool is_stopped() const override
      {
         return false;
      }

      void close() override
      {
      }
};



#endif // INFRASTRUCTURE_CONNECTION_STUB_HPP
