// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <micrortps/client/client.h>
#include <stdio.h>

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UDP_TRANSPORT_MTU * STREAM_HISTORY

void on_topic(Session* session, mrObjectId id, uint16_t request_id, StreamId stream_id, MicroBuffer* mb, void* args)
{
    (void) session; (void) id; (void) request_id; (void) mb; (void) stream_id; (void) args;
    //printf("Topic received!\n");
    /* HelloWorld topic;
    deserialize_HelloWorld_topic(serialized_topic, &topic);

    printf("Receive topic: %s, count: %i\n", topic.message, topic.index); */
}

int main(int args, char** argv)
{
    (void) args; (void) argv;

    // Transport
    UDPTransport transport;
    if(!init_udp_transport(&transport, "127.0.0.1", 2019))
    {
        printf("Error at create transport.\n");
        return 1;
    }

    // Session
    Session session;
    if(!create_session(&session, 0x81, 0xAABBCCDD, &transport.comm))
    {
        printf("Error at create session.\n");
        return 1;
    }
    set_topic_callback(&session, on_topic, NULL);

    // Streams
    uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
    StreamId reliable_out = create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    StreamId reliable_in = create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Create entities
    mrObjectId participant_id = create_object_id(0x01, PARTICIPANT_ID);
    char* participant_ref = "default participant";
    uint16_t participant_req = write_create_participant_ref(&session, reliable_out, participant_id, participant_ref, 0);

    mrObjectId topic_id = create_object_id(0x01, TOPIC_ID);
    char* topic_xml = "<dds><topic><name>HelloWorldTopic</name><dataType>HelloWorld</dataType></topic></dds>";
    uint16_t topic_req = write_configure_topic_xml(&session, reliable_out, topic_id, participant_id, topic_xml, 0);

    mrObjectId subscriber_id = create_object_id(0x01, SUBSCRIBER_ID);
    char* subscriber_xml = {"<subscriber name=\"MySubscriber\""};
    uint16_t subscriber_req = write_configure_subscriber_xml(&session, reliable_out, subscriber_id, participant_id, subscriber_xml, 0);

    mrObjectId datareader_id = create_object_id(0x01, DATAREADER_ID);
    char* datareader_xml = {"<profiles><subscriber profile_name=\"default_xrce_subscriber_profile\"><topic><kind>NO_KEY</kind><name>HelloWorldTopic</name><dataType>HelloWorld</dataType><historyQos><kind>KEEP_LAST</kind><depth>5</depth></historyQos><durability><kind>TRANSIENT_LOCAL</kind></durability></topic></subscriber></profiles>"};
    uint16_t datareader_req = write_configure_datareader_xml(&session, reliable_out, datareader_id, subscriber_id, datareader_xml, 0);

    // Send create entities message and wait its status
    uint8_t status[4];
    uint16_t requests[4] = {participant_req, topic_req, subscriber_req, datareader_req};
    run_session_until_status(&session, 1000, requests, status, 4);
    if(!check_status_list_ok(status, 4))
    {
        printf("Error at create entities: participant: %i topic: %i subscriber: %i datareader: %i\n", status[0], status[1], status[2], status[3]);
        return 1;
    }

    // Read 10 topics
    for(unsigned i = 0; i < 100; ++i)
    {
        uint8_t read_data_status;
        uint16_t read_data_req = write_read_data(&session, reliable_out, datareader_id, reliable_in, MR_FORMAT_DATA, NULL);
        run_session_until_status(&session, 3000, &read_data_req, &read_data_status, 1);
    }

    // Delete resources
    delete_session(&session);

    return 0;
}










/*
printf("Usage: program <command>\n");
printf("List of commands:\n");
printf("    serial device\n");
printf("    udp agent_ip agent_port\n");
printf("    help\n");
return 1;
*/
