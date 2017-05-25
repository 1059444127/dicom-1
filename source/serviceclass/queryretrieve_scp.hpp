#ifndef QUERYRETRIEVE_SCP_HPP
#define QUERYRETRIEVE_SCP_HPP

#include <string>
#include <functional>
#include <memory>
#include <thread>

#include "network/upperlayer/upperlayer.hpp"
#include "network/dimse/dimse_pm.hpp"
#include "network/dimse/sop_class.hpp"
#include "network/dimse/association_definition.hpp"
#include "serviceclass.hpp"
#include "storage_scu.hpp"

namespace dicom
{

namespace serviceclass
{


class queryretrieve_scp;

class storage_scu_thread;

class queryretrieve_scp : public Iserviceclass
{
   public:
      queryretrieve_scp(std::string calling_ae, std::string called_ae,
                        short port, dicom::data::dictionary::dictionary& dict,
                        std::function<void(queryretrieve_scp*, dicom::data::dataset::commandset_data, std::shared_ptr<dicom::data::dataset::iod>)> handler);

      ~queryretrieve_scp();

      void start_listen();

      dicom::network::upperlayer::scp& get_scp();

      virtual void run() override;

      void send_image(boost::optional<dicom::data::dataset::iod> data);

   private:
      void handle_cfind(dicom::network::dimse::dimse_pm* pm,
                        dicom::data::dataset::commandset_data command,
                        std::unique_ptr<dicom::data::dataset::iod> data);

      boost::optional<data::dataset::iod> handle_cmove(queryretrieve_scp* scp,
                        dicom::data::dataset::commandset_data command,
                        std::shared_ptr<dicom::data::dataset::iod> data);

      dicom::data::dictionary::dictionary& dict;
      dicom::network::dimse::SOP_class sop_class;
      dicom::network::dimse::association_definition assoc_def;
      dicom::network::upperlayer::scp scp;
      dicom::network::dimse::dimse_pm_manager dimse_pm;

      std::unique_ptr<storage_scu_thread> storage_operation;
      std::unique_ptr<std::thread> storage_thread;

      std::function<void(queryretrieve_scp*, dicom::data::dataset::commandset_data, std::shared_ptr<dicom::data::dataset::iod>)> handler;
      std::shared_ptr<dicom::data::dataset::iod> filter_data;
      boost::optional<dicom::data::dataset::iod> response_data;
};

}

}

#endif // QUERYRETRIEVE_SCP_HPP
