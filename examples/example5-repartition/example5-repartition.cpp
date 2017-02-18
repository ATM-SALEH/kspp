#include <iostream>
#include <string>
#include <chrono>
#include <kspp/codecs/text_codec.h>
#include <kspp/topology_builder.h>
#include <kspp/processors/kafka_source.h>
#include <kspp/processors/transform.h>
#include <kspp/processors/count.h>
#include <kspp/processors/repartition.h>
#include <kspp/processors/ktable.h>
#include <kspp/state_stores/rocksdb_store.h>
#include <kspp/algorithm.h>
#include <kspp/sinks/kafka_sink.h>
#include <kspp/sinks/stream_sink.h>
#include <assert.h>

#define PARTITION 0



int main(int argc, char **argv) {
  auto codec = std::make_shared<kspp::text_codec>();
  auto app_info = std::make_shared<kspp::app_info>("kspp-examples", "example5-repartition");
  auto builder = kspp::topology_builder(app_info, "localhost");
  
  auto partition_list = kspp::parse_partition_list("[0,1,2,3,4,5,6,7]"); // tmp fix waiting for partition data in topology_builder.../ topology

  {
    auto topology = builder.create_topic_topology();
    auto sink = topology->create_topic_sink<kspp::kafka_topic_sink<int, std::string, kspp::text_codec>>("kspp_example5_usernames", codec);
    kspp::produce(*sink, 1, "user_1");
    kspp::produce(*sink, 1, "user_1");
    kspp::produce(*sink, 2, "user_2");
    kspp::produce(*sink, 3, "user_3");
    kspp::produce(*sink, 4, "user_4");
    kspp::produce(*sink, 5, "user_5");
    kspp::produce(*sink, 6, "user_6");
    kspp::produce(*sink, 7, "user_7");
    kspp::produce(*sink, 8, "user_8");
    kspp::produce(*sink, 9, "user_9");
    kspp::produce(*sink, 10, "user_10");
  }

  {
    auto topology = builder.create_topic_topology();
    auto sink = topology->create_topic_sink<kspp::kafka_topic_sink<int, int, kspp::text_codec>>("kspp_example5_user_channel", codec); // <user_id, channel_id>
    kspp::produce(*sink, 1, 1);
    kspp::produce(*sink, 2, 1);
    kspp::produce(*sink, 3, 1);
    kspp::produce(*sink, 4, 1);
    kspp::produce(*sink, 5, 2);
    kspp::produce(*sink, 6, 2);
    kspp::produce(*sink, 7, 2);
    kspp::produce(*sink, 8, 2);
    kspp::produce(*sink, 9, 2);
    kspp::produce(*sink, 10, 2);
  }

  {
    auto topology = builder.create_topic_topology();
    auto sink = topology->create_topic_sink<kspp::kafka_topic_sink<int, std::string, kspp::text_codec>>("kspp_example5_channel_names", codec);
    kspp::produce(*sink, 1, "channel1");
    kspp::produce(*sink, 2, "channel2");
  }

  {
    auto topology = builder.create_topic_topology(); 
    auto sources = topology->create_partition_processors<kspp::kafka_source<int, std::string, kspp::text_codec>>(partition_list, "kspp_example5_usernames", codec);
    topology->create_partition_processors<kspp::stream_sink<int, std::string>>(sources, &std::cerr);
    topology->start(-2);
    topology->flush();

  }

  {
    auto topology = builder.create_topic_topology();
    auto topic_sink = topology->create_topic_sink<kspp::kafka_topic_sink<int, std::string, kspp::text_codec>>("kspp_example5_usernames.per-channel", codec);
    auto sources = topology->create_partition_processors<kspp::kafka_source<int, std::string, kspp::text_codec>>(partition_list, "kspp_example5_usernames", codec);
    auto routing_sources = topology->create_partition_processors<kspp::kafka_source<int, int, kspp::text_codec>>(partition_list, "kspp_example5_user_channel", codec);
    auto routing_tables = topology->create_partition_processors<kspp::ktable<int, int, kspp::rocksdb_store, kspp::text_codec>>(routing_sources, codec);
    auto partition_repartition = topology->create_partition_processors<kspp::repartition_by_table<int, std::string, int, kspp::text_codec>>(sources, routing_tables, topic_sink);

    topology->init_metrics();
    topology->start(-2);
    topology->flush();
    topology->for_each_metrics([](kspp::metric& m) {
      std::cerr << "metrics: " << m.name() << " : " << m.value() << std::endl;
    });
  }


  {
    auto topology = builder.create_topic_topology();
    auto sources = topology->create_partition_processors<kspp::kafka_source<int, std::string, kspp::text_codec>>(partition_list, "kspp_example5_usernames.per-channel", codec);
    topology->create_partition_processors<kspp::stream_sink<int, std::string>>(sources, &std::cerr);
    topology->start(-2);
    topology->flush();
    topology->for_each_metrics([](kspp::metric& m) {
      std::cerr << "metrics: " << m.name() << " : " << m.value() << std::endl;
    });
  }
}
