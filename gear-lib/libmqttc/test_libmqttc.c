/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "libmqttc.h"
#include <libtime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>


#define LOGA_DEBUG 0
#define LOGA_INFO 1
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static volatile MessageData* test1_message_data = NULL;
static mqtt_msg pubmsg;
static volatile MessageData* test2_message_data = NULL;

void usage(void)
{
    printf("help!!\n");
    exit(EXIT_FAILURE);
}

struct Options
{
    char* host;         /**< connection to system under test. */
    int port;
    char* proxy_host;
    int proxy_port;
    int verbose;
    int test_no;
    int mqtt_version;
    int iterations;
} options = {
    "localhost",
    //"127.0.0.1",
    1883,
    "localhost",
    //"127.0.0.1",
    1885,
    0, //verbose
    0, //test_no
    4,
    1,
};

void getopts(int argc, char** argv)
{
    int count = 1;

    while (count < argc) {
        if (strcmp(argv[count], "--test_no") == 0) {
            if (++count < argc)
                options.test_no = atoi(argv[count]);
            else
                usage();
        } else if (strcmp(argv[count], "--host") == 0) {
            if (++count < argc) {
                options.host = argv[count];
                printf("\nSetting host to %s\n", options.host);
            } else
                usage();
        } else if (strcmp(argv[count], "--port") == 0) {
            if (++count < argc) {
                options.port = atoi(argv[count]);
                printf("\nSetting port to %d\n", options.port);
            } else
                usage();
        } else if (strcmp(argv[count], "--proxy_host") == 0) {
            if (++count < argc) {
                options.proxy_host = argv[count];
                printf("\nSetting proxy_host to %s\n", options.proxy_host);
            } else
                usage();
        } else if (strcmp(argv[count], "--proxy_port") == 0) {
            if (++count < argc) {
                options.proxy_port = atoi(argv[count]);
                printf("\nSetting proxy_port to %d\n", options.proxy_port);
            } else
                usage();
        } else if (strcmp(argv[count], "--MQTTversion") == 0) {
            if (++count < argc) {
                options.mqtt_version = atoi(argv[count]);
                printf("setting MQTT version to %d\n", options.mqtt_version);
            } else
                usage();
        } else if (strcmp(argv[count], "--iterations") == 0) {
            if (++count < argc)
                options.iterations = atoi(argv[count]);
            else
                usage();
        } else if (strcmp(argv[count], "--verbose") == 0) {
            options.verbose = 1;
            printf("\nSetting verbose on\n");
        }
        count++;
    }
}

void MyLog(int LOGA_level, char* format, ...)
{
    static char msg_buf[256];
    va_list args;
    struct timeb ts;

    struct tm *timeinfo;

    //if (LOGA_level == LOGA_DEBUG && options.verbose == 0)
     //   return;

    ftime(&ts);
    timeinfo = localtime(&ts.time);
    strftime(msg_buf, 80, "%Y%m%d %H%M%S", timeinfo);

    sprintf(&msg_buf[strlen(msg_buf)], ".%.3hu ", ts.millitm);

    va_start(args, format);
    vsnprintf(&msg_buf[strlen(msg_buf)], sizeof(msg_buf) - strlen(msg_buf), format, args);
    va_end(args);

    printf("%s\n", msg_buf);
    fflush(stdout);
}


#define START_TIME_TYPE struct timeval
/* TODO - unused - remove? static struct timeval start_time; */
START_TIME_TYPE start_clock(void)
{
    struct timeval start_time;
    gettimeofday(&start_time, NULL);
    return start_time;
}


long elapsed(START_TIME_TYPE start_time)
{
    struct timeval now, res;

    gettimeofday(&now, NULL);
    timersub(&now, &start_time, &res);
    return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}

#define assert(a, b, c, d) myassert(__FILE__, __LINE__, a, b, c, d)
#define assert1(a, b, c, d, e) myassert(__FILE__, __LINE__, a, b, c, d, e)

int tests = 0;
int failures = 0;
FILE* xml;
START_TIME_TYPE global_start_time;
struct timeval g_start_time;
char output[3000];
char* cur_output = output;

void write_test_result(void)
{
    //long duration = elapsed(global_start_time);
    uint64_t duration = time_elapsed_ms(&g_start_time);

    fprintf(xml, " time=\"%ld.%.3ld\" >\n", duration / 1000, duration % 1000);
    if (cur_output != output) {
        fprintf(xml, "%s", output);
        cur_output = output;
    }
    fprintf(xml, "</testcase>\n");
}

void myassert(char* filename, int lineno, char* description, int value, char* format, ...)
{
    ++tests;
    if (!value) {
        va_list args;

        ++failures;
        MyLog(LOGA_INFO, "Assertion failed, file %s, line %d, description: %s\n", filename, lineno, description);

        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        cur_output += sprintf(cur_output, "<failure type=\"%s\">file %s, line %d </failure>\n",
                        description, filename, lineno);
    } else
        MyLog(LOGA_DEBUG, "Assertion succeeded, file %s, line %d, description: %s", filename, lineno, description);
}

void messageArrived(MessageData* md)
{
    test1_message_data = md;
    mqtt_msg* m = md->message;

    assert("Good message lengths", pubmsg.payloadlen == m->payloadlen,
                    "payloadlen was %d", m->payloadlen);

    if (pubmsg.payloadlen == m->payloadlen)
            assert("Good message contents", memcmp(m->payload, pubmsg.payload, m->payloadlen) == 0,
                            "payload was %s", m->payload);
}


/*********************************************************************

Test1: single-threaded client

*********************************************************************/
void test1_sendAndReceive(mqtt_client* c, int qos, char* test_topic)
{
    int i = 0;
    int iterations = 5;
    int rc;
    int wait_seconds;

    MyLog(LOGA_DEBUG, "%d messages at QoS %d", iterations, qos);
    memset(&pubmsg, '\0', sizeof(pubmsg));
    pubmsg.payload = "a much longer message that we can shorten to the extent that we need to payload up to 11";
    pubmsg.payloadlen = 11;
    pubmsg.qos = qos;
    pubmsg.retained = 0;
    pubmsg.dup = 0;

    for (i = 0; i < iterations; ++i) {
        test1_message_data = NULL;
        rc = mqtt_publish(c, test_topic, &pubmsg);
        assert("Good rc from publish", rc == MQTT_SUCCESS, "rc was %d\n", rc);

        /* wait for the message to be received */
        wait_seconds = 10;
        while ((test1_message_data == NULL) && (wait_seconds-- > 0)) {
            mqtt_yield(c, 100);
        }
        assert("Message Arrived", wait_seconds > 0, "Time out waiting for message %d\n", i);

        if (!test1_message_data)
            printf("No message received within timeout period\n");
    }

    /* wait to receive any outstanding messages */
    wait_seconds = 2;
    while (wait_seconds-- > 0) {
        mqtt_yield(c, 1000);
    }
}

int test1(struct Options options)
{
    int subsqos = 2;
    mqtt_client c;
    int rc = 0;
    char* test_topic = "C client test1";

    fprintf(xml, "<testcase classname=\"test1\" name=\"single threaded client using receive\"");
    //global_start_time = start_clock();
    gettimeofday(&g_start_time, NULL);
    failures = 0;
    MyLog(LOGA_INFO, "Starting test 1 - single threaded client using receive");

    mqtt_client_init(&c, options.host, options.port);

    mqtt_suback_data tmp_subback_data;
    mqtt_connack_data tmp_data;
    mqtt_pkt_conn_data data = mqtt_pkt_conn_data_initializer;
    data.willFlag = 1;
    data.mqtt_version = options.mqtt_version;
    data.clientID.cstring = "single-threaded-test";
    data.username.cstring = "testuser";
    data.password.cstring = "testpassword";
    data.keepAliveInterval = 20;
    data.cleansession = 1;
    data.will.message.cstring = "will message";
    data.will.qos = 1;
    data.will.retained = 0;
    data.will.topicName.cstring = "will topic";

    MyLog(LOGA_DEBUG, "Connecting");
    rc = mqtt_connect(&c, &data, &tmp_data);
    assert("Good rc from connect", rc == MQTT_SUCCESS, "rc was %d", rc);
    if (rc != MQTT_SUCCESS)
        goto exit;

    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived, &tmp_subback_data);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);

    test1_sendAndReceive(&c, 0, test_topic);
    test1_sendAndReceive(&c, 1, test_topic);
    test1_sendAndReceive(&c, 2, test_topic);

    MyLog(LOGA_DEBUG, "Stopping\n");

    rc = mqtt_unsubscribe(&c, test_topic);
    assert("Unsubscribe successful", rc == MQTT_SUCCESS, "rc was %d", rc);
    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

#if 0
    /* Just to make sure we can connect again */
    //NetworkConnect(&c.ipstack, options.host, options.port);
    rc = mqtt_connect(&c, &data, &tmp_data);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);
    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);
#endif

exit:
    MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
                    (failures == 0) ? "passed" : "failed", tests, failures);
    write_test_result();
    mqtt_client_deinit(&c);
    return failures;
}


/*********************************************************************

Test 2: connack return data

sessionPresent

*********************************************************************/
int test2(struct Options options)
{
    int subsqos = 2;
    mqtt_client c;
    int rc = 0;
    char* test_topic = "C client test2";
    mqtt_connack_data connack;
    mqtt_suback_data suback;

    fprintf(xml, "<testcase classname=\"test2\" name=\"connack return data\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting test 2 - connack return data");

    mqtt_client_init(&c, options.host, options.port);

    mqtt_connack_data tmp_data;
    mqtt_pkt_conn_data data = mqtt_pkt_conn_data_initializer;
    data.willFlag = 1;
    data.mqtt_version = options.mqtt_version;
    data.clientID.cstring = "connack-return-data";
    data.username.cstring = "testuser";
    data.password.cstring = "testpassword";

    data.keepAliveInterval = 20;
    data.cleansession = 1;

    data.will.message.cstring = "will message";
    data.will.qos = 1;
    data.will.retained = 0;
    data.will.topicName.cstring = "will topic";

    MyLog(LOGA_DEBUG, "Connecting");
    rc = mqtt_connect(&c, &data, &tmp_data);
    assert("Good rc from connect", rc == MQTT_SUCCESS, "rc was %d", rc);
    if (rc != MQTT_SUCCESS)
            goto exit;

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* now connect cleansession false */
    //NetworkConnect(&c.ipstack, options.host, options.port);
    data.cleansession = 0;
    rc = mqtt_connect(&c, &data, &connack);
    assert("Good rc from connect", rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 0", connack.sessionPresent == 0,
                    "sessionPresent was %d", connack.sessionPresent);

    /* set up some state */
    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived, &suback);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);
    assert("Good granted QoS", suback.grantedQoS == subsqos,
                    "granted QoS was %d", suback.grantedQoS);

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* Connect and check sessionPresent */
    //NetworkConnect(&c.ipstack, options.host, options.port);
    rc = mqtt_connect(&c, &data, &connack);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 1", connack.sessionPresent == 1,
                    "sessionPresent was %d", connack.sessionPresent);

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* Connect and check sessionPresent is cleared */
    data.cleansession = 1;
    //NetworkConnect(&c.ipstack, options.host, options.port);
    rc = mqtt_connect(&c, &data, &connack);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 0", connack.sessionPresent == 0,
                    "sessionPresent was %d", connack.sessionPresent);

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

exit:
    MyLog(LOGA_INFO, "TEST1: test %s. %d tests run, %d failures.",
                    (failures == 0) ? "passed" : "failed", tests, failures);
    write_test_result();
    return failures;
}

/*********************************************************************

Test 3: client session state

*********************************************************************/

void messageArrived2(MessageData* md)
{
    test2_message_data = md;
    mqtt_msg *m = md->message;

    assert("Good message lengths", pubmsg.payloadlen == m->payloadlen,
         "payloadlen was %d", m->payloadlen);

    if (pubmsg.payloadlen == m->payloadlen)
        assert("Good message contents", memcmp(m->payload, pubmsg.payload, m->payloadlen) == 0,
          "payload was %s", m->payload);
}


int check_subs_exist(mqtt_client* c, const char* test_topic, int which)
{
    int rc = MQTT_FAILURE;
    int wait_seconds = 0;

    memset(&pubmsg, '\0', sizeof(pubmsg));
    pubmsg.payload = (void*)"a much longer message that we can shorten to the extent that we need to payload up to 11";
    pubmsg.payloadlen = 11;
    pubmsg.qos = MQTT_QOS2;
    pubmsg.retained = 0;
    pubmsg.dup = 0;

    test1_message_data = test2_message_data = NULL;
    rc = mqtt_publish(c, test_topic, &pubmsg);
    assert("Good rc from publish", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* wait for the message to be received */
    wait_seconds = 10;
    while (wait_seconds-- > 0)
    {
        mqtt_yield(c, 100);
    }

    rc = (((which == 1 || which == 3) && test1_message_data) ||
         (which == 2 && test1_message_data == NULL)) ? MQTT_SUCCESS : MQTT_FAILURE;
    assert("test1 subscription", rc == MQTT_SUCCESS, "test1_message_data %p\n",
            test1_message_data);
    rc = (((which == 2 || which == 3) && test2_message_data) ||
         (which == 1 && test2_message_data == NULL)) ? MQTT_SUCCESS : MQTT_FAILURE;
    assert("test2 subscription", rc == MQTT_SUCCESS, "test2_message_data %p\n",
             test2_message_data);
    return rc;
}


int test3(struct Options options)
{
    enum mqtt_qos subsqos = MQTT_QOS2;
    mqtt_client c;
    int rc;
    const char* test_topic = "C client test3";
    int wait_seconds = 0;
    mqtt_connack_data connack;
    mqtt_suback_data suback;

    fprintf(xml, "<testcase classname=\"test3\" name=\"session state\"");
    global_start_time = start_clock();
    failures = 0;
    MyLog(LOGA_INFO, "Starting test 3 - session state");

    mqtt_client_init(&c, options.host, options.port);

    mqtt_pkt_conn_data data = mqtt_pkt_conn_data_initializer;
    data.willFlag = 1;
    data.mqtt_version = options.mqtt_version;
    data.clientID.cstring = (char*)"connack-session-state";
    data.username.cstring = (char*)"testuser";
    data.password.cstring = (char*)"testpassword";

    data.keepAliveInterval = 10;
    data.cleansession = 1;

    data.will.message.cstring = (char*)"will message";
    data.will.qos = 1;
    data.will.retained = 0;
    data.will.topicName.cstring = (char*)"will topic";

    assert("Not connected", mqtt_is_connected(&c) == 0,
                    "isconnected was %d", mqtt_is_connected(&c));

    MyLog(LOGA_DEBUG, "Connecting");
#if 0
    rc = NetworkConnect(&c.ipstack, options.host, options.port);
    assert("Good rc from TCP connect", rc == MQTT_SUCCESS, "rc was %d", rc);
    if (rc != MQTT_SUCCESS)
            goto exit;
#endif

    rc = mqtt_connect(&c, &data, &connack);
    assert("Good rc from connect", rc == MQTT_SUCCESS, "rc was %d", rc);
    if (rc != MQTT_SUCCESS)
            goto exit;

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 0", connack.sessionPresent == 0,
                    "sessionPresent was %d", connack.sessionPresent);

    assert("Good rc in connack", mqtt_is_connected(&c) == 1,
                    "isconnected was %d", mqtt_is_connected(&c));

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* reconnect with cleansession false */
    data.cleansession = 0;
    //rc = NetworkConnect(&c.ipstack, options.proxy_host, options.proxy_port);
    assert("TCP connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);
    rc = mqtt_connect(&c, &data, &connack);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 0", connack.sessionPresent == 0,
                    "sessionPresent was %d", connack.sessionPresent);

    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived, &suback);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);
    assert("Granted QoS rc from subscribe", suback.grantedQoS == MQTT_QOS2,
                    "rc was %d", suback.grantedQoS);

    check_subs_exist(&c, test_topic, 1);

    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived2, &suback);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);
    assert("Granted QoS rc from subscribe", suback.grantedQoS == MQTT_QOS2,
                    "rc was %d", suback.grantedQoS);

    check_subs_exist(&c, test_topic, 2);

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* reconnect with cleansession false */
    data.cleansession = 0;
    //rc = NetworkConnect(&c.ipstack, options.proxy_host, options.proxy_port);
    assert("TCP connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);
    rc = mqtt_connect(&c, &data, &connack);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 1", connack.sessionPresent == 1,
                    "sessionPresent was %d", connack.sessionPresent);

    check_subs_exist(&c, test_topic, 2);

    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived, &suback);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);
    assert("Granted QoS rc from subscribe", suback.grantedQoS == MQTT_QOS2,
                    "rc was %d", suback.grantedQoS);

    check_subs_exist(&c, test_topic, 1);

    // cause a connection MQTT_FAILURE
    memset(&pubmsg, '\0', sizeof(pubmsg));
    pubmsg.payload = (void*)"TERMINATE";
    pubmsg.payloadlen = strlen((char*)pubmsg.payload);
    pubmsg.qos = MQTT_QOS0;
    pubmsg.retained = 0;
    pubmsg.dup = 0;
    rc = mqtt_publish(&c, "MQTTSAS topic", &pubmsg);
    assert("Good rc from publish", rc == MQTT_SUCCESS, "rc was %d", rc);

    // wait for failure to be noticed by keepalive
    wait_seconds = 20;
    while (mqtt_is_connected(&c) && (wait_seconds-- > 0))
    {
            mqtt_yield(&c, 1000);
    }
    assert("Disconnected", !mqtt_is_connected(&c), "isConnected was %d",
                    mqtt_is_connected(&c));
    rc = mqtt_disconnect(&c);

    /* reconnect with cleansession false */
    data.cleansession = 0;
    //rc = NetworkConnect(&c.ipstack, options.host, options.port);
    assert("TCP connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);
    rc = mqtt_connect(&c, &data, &connack);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 1", connack.sessionPresent == 1,
                    "sessionPresent was %d", connack.sessionPresent);

    check_subs_exist(&c, test_topic, 1);

    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived2, &suback);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);
    assert("Granted QoS rc from subscribe", suback.grantedQoS == MQTT_QOS2,
                    "rc was %d", suback.grantedQoS);

    check_subs_exist(&c, test_topic, 2);

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

    /* reconnect with cleansession true to clean up both server and client state */
    data.cleansession = 1;
    //rc = NetworkConnect(&c.ipstack, options.host, options.port);
    assert("TCP connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);
    rc = mqtt_connect(&c, &data, &connack);
    assert("Connect successful",  rc == MQTT_SUCCESS, "rc was %d", rc);

    assert("Good rc in connack", connack.rc == 0, "rc was %d", connack.rc);
    assert("Session present is 0", connack.sessionPresent == 0,
                    "sessionPresent was %d", connack.sessionPresent);

    rc = mqtt_subscribe(&c, test_topic, subsqos, messageArrived2, &suback);
    assert("Good rc from subscribe", rc == MQTT_SUCCESS, "rc was %d", rc);
    assert("Granted QoS rc from subscribe", suback.grantedQoS == MQTT_QOS2,
                    "rc was %d", suback.grantedQoS);

    check_subs_exist(&c, test_topic, 2);

    rc = mqtt_disconnect(&c);
    assert("Disconnect successful", rc == MQTT_SUCCESS, "rc was %d", rc);

exit:
    MyLog(LOGA_INFO, "TEST2: test %s. %d tests run, %d failures.",
                    (failures == 0) ? "passed" : "failed", tests, failures);
    write_test_result();
    return failures;
}

#if 0
/*********************************************************************

Test 4: connectionLost and will message

*********************************************************************/
mqtt_client test6_c1, test6_c2;
volatile int test6_will_message_arrived = 0;
volatile int test6_connection_lost_called = 0;

void test6_connectionLost(void* context, char* cause)
{
    mqtt_client c = (mqtt_client)context;
    printf("%s -> Callback: connection lost\n", (c == test6_c1) ? "Client-1" : "Client-2");
    test6_connection_lost_called = 1;
}

void test6_deliveryComplete(void* context, mqtt_client_deliveryToken token)
{
    printf("Client-2 -> Callback: publish complete for token %d\n", token);
}

char* test6_will_topic = "C Test 2: will topic";
char* test6_will_message = "will message from Client-1";

int test6_messageArrived(void* context, char* topicName, int topicLen, mqtt_client_message* m)
{
    mqtt_client c = (mqtt_client)context;
    printf("%s -> Callback: message received on topic '%s' is '%.*s'.\n",
                    (c == test6_c1) ? "Client-1" : "Client-2", topicName, m->payloadlen, (char*)(m->payload));
    if (c == test6_c2 && strcmp(topicName, test6_will_topic) == 0 && memcmp(m->payload, test6_will_message, m->payloadlen) == 0)
            test6_will_message_arrived = 1;
    mqtt_client_free(topicName);
    mqtt_client_freeMessage(&m);
    return 1;
}


int test6(struct Options options)
{
    char* testname = "test6";
    mqtt_client_connectOptions opts = mqtt_client_connectOptions_initializer;
    mqtt_client_willOptions wopts =  mqtt_client_willOptions_initializer;
    mqtt_client_connectOptions opts2 = mqtt_client_connectOptions_initializer;
    int rc, count;
    char* mqttsas_topic = "MQTTSAS topic";

    failures = 0;
    MyLog(LOGA_INFO, "Starting test 6 - connectionLost and will messages");
    fprintf(xml, "<testcase classname=\"test1\" name=\"connectionLost and will messages\"");
    global_start_time = start_clock();

    opts.keepAliveInterval = 2;
    opts.cleansession = 1;
    opts.mqtt_version = MQTTVERSION_3_1_1;
    opts.will = &wopts;
    opts.will->message = test6_will_message;
    opts.will->qos = 1;
    opts.will->retained = 0;
    opts.will->topicName = test6_will_topic;
    if (options.haconnections != NULL)
    {
            opts.serverURIs = options.haconnections;
            opts.serverURIcount = options.hacount;
    }

    /* Client-1 with Will options */
    rc = mqtt_client_create(&test6_c1, options.proxy_connection, "Client_1", MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);
    assert("good rc from create", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);
    if (rc != MQTTCLIENT_SUCCESS)
            goto exit;

    rc = mqtt_client_setCallbacks(test6_c1, (void*)test6_c1, test6_connectionLost, test6_messageArrived, test6_deliveryComplete);
    assert("good rc from setCallbacks",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);
    if (rc != MQTTCLIENT_SUCCESS)
            goto exit;

    /* Connect to the broker */
    rc = mqtt_client_connect(test6_c1, &opts);
    assert("good rc from connect",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);
    if (rc != MQTTCLIENT_SUCCESS)
            goto exit;

    /* Client - 2 (multi-threaded) */
    rc = mqtt_client_create(&test6_c2, options.connection, "Client_2", MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);
    assert("good rc from create",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    /* Set the callback functions for the client */
    rc = mqtt_client_setCallbacks(test6_c2, (void*)test6_c2, test6_connectionLost, test6_messageArrived, test6_deliveryComplete);
    assert("good rc from setCallbacks",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    /* Connect to the broker */
    opts2.keepAliveInterval = 20;
    opts2.cleansession = 1;
    MyLog(LOGA_INFO, "Connecting Client_2 ...");
    rc = mqtt_client_connect(test6_c2, &opts2);
    assert("Good rc from connect", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    rc = mqtt_client_subscribe(test6_c2, test6_will_topic, 2);
    assert("Good rc from subscribe", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    /* now send the command which will break the connection and cause the will message to be sent */
    rc = mqtt_client_publish(test6_c1, mqttsas_topic, (int)strlen("TERMINATE"), "TERMINATE", 0, 0, NULL);
    assert("Good rc from publish", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    MyLog(LOGA_INFO, "Waiting to receive the will message");
    count = 0;
    while (++count < 40)
    {
            sleep(1);
            if (test6_will_message_arrived == 1 && test6_connection_lost_called == 1)
                    break;
    }
    assert("will message arrived", test6_will_message_arrived == 1,
                    "will_message_arrived was %d\n", test6_will_message_arrived);
    assert("connection lost called", test6_connection_lost_called == 1,
                    "connection_lost_called %d\n", test6_connection_lost_called);

    rc = mqtt_client_unsubscribe(test6_c2, test6_will_topic);
    assert("Good rc from unsubscribe", rc == MQTTCLIENT_SUCCESS, "rc was %d", rc);

    rc = mqtt_client_isConnected(test6_c2);
    assert("Client-2 still connected", rc == 1, "isconnected is %d", rc);

    rc = mqtt_client_isConnected(test6_c1);
    assert("Client-1 not connected", rc == 0, "isconnected is %d", rc);

    rc = mqtt_client_disconnect(test6_c2, 100L);
    assert("Good rc from disconnect", rc == MQTTCLIENT_SUCCESS, "rc was %d", rc);

    mqtt_client_destroy(&test6_c1);
    mqtt_client_destroy(&test6_c2);

exit:
    MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.\n",
                    (failures == 0) ? "passed" : "failed", testname, tests, failures);
    write_test_result();
    return failures;
}

int test6a(struct Options options)
{
    char* testname = "test6a";
    mqtt_client_connectOptions opts = mqtt_client_connectOptions_initializer;
    mqtt_client_willOptions wopts =  mqtt_client_willOptions_initializer;
    mqtt_client_connectOptions opts2 = mqtt_client_connectOptions_initializer;
    int rc, count;
    char* mqttsas_topic = "MQTTSAS topic";

    failures = 0;
    MyLog(LOGA_INFO, "Starting test 6 - connectionLost and binary will messages");
    fprintf(xml, "<testcase classname=\"test1\" name=\"connectionLost and binary will messages\"");
    global_start_time = start_clock();

    opts.keepAliveInterval = 2;
    opts.cleansession = 1;
    opts.mqtt_version = MQTTVERSION_3_1_1;
    opts.will = &wopts;
    opts.will->payload.data = test6_will_message;
    opts.will->payload.len = strlen(test6_will_message) + 1;
    opts.will->qos = 1;
    opts.will->retained = 0;
    opts.will->topicName = test6_will_topic;
    if (options.haconnections != NULL) {
        opts.serverURIs = options.haconnections;
        opts.serverURIcount = options.hacount;
    }

    /* Client-1 with Will options */
    rc = mqtt_client_create(&test6_c1, options.proxy_connection, "Client_1", MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);
    assert("good rc from create", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);
    if (rc != MQTTCLIENT_SUCCESS)
        goto exit;

    rc = mqtt_client_setCallbacks(test6_c1, (void*)test6_c1, test6_connectionLost, test6_messageArrived, test6_deliveryComplete);
    assert("good rc from setCallbacks",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);
    if (rc != MQTTCLIENT_SUCCESS)
        goto exit;

    /* Connect to the broker */
    rc = mqtt_client_connect(test6_c1, &opts);
    assert("good rc from connect",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);
    if (rc != MQTTCLIENT_SUCCESS)
        goto exit;

    /* Client - 2 (multi-threaded) */
    rc = mqtt_client_create(&test6_c2, options.connection, "Client_2", MQTTCLIENT_PERSISTENCE_DEFAULT, NULL);
    assert("good rc from create",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    /* Set the callback functions for the client */
    rc = mqtt_client_setCallbacks(test6_c2, (void*)test6_c2, test6_connectionLost, test6_messageArrived, test6_deliveryComplete);
    assert("good rc from setCallbacks",  rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    /* Connect to the broker */
    opts2.keepAliveInterval = 20;
    opts2.cleansession = 1;
    MyLog(LOGA_INFO, "Connecting Client_2 ...");
    rc = mqtt_client_connect(test6_c2, &opts2);
    assert("Good rc from connect", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    rc = mqtt_client_subscribe(test6_c2, test6_will_topic, 2);
    assert("Good rc from subscribe", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    /* now send the command which will break the connection and cause the will message to be sent */
    rc = mqtt_client_publish(test6_c1, mqttsas_topic, (int)strlen("TERMINATE"), "TERMINATE", 0, 0, NULL);
    assert("Good rc from publish", rc == MQTTCLIENT_SUCCESS, "rc was %d\n", rc);

    MyLog(LOGA_INFO, "Waiting to receive the will message");
    count = 0;
    while (++count < 40) {
        sleep(1);
        if (test6_will_message_arrived == 1 && test6_connection_lost_called == 1)
                break;
    }
    assert("will message arrived", test6_will_message_arrived == 1,
                    "will_message_arrived was %d\n", test6_will_message_arrived);
    assert("connection lost called", test6_connection_lost_called == 1,
                    "connection_lost_called %d\n", test6_connection_lost_called);

    rc = mqtt_client_unsubscribe(test6_c2, test6_will_topic);
    assert("Good rc from unsubscribe", rc == MQTTCLIENT_SUCCESS, "rc was %d", rc);

    rc = mqtt_client_isConnected(test6_c2);
    assert("Client-2 still connected", rc == 1, "isconnected is %d", rc);

    rc = mqtt_client_isConnected(test6_c1);
    assert("Client-1 not connected", rc == 0, "isconnected is %d", rc);

    rc = mqtt_client_disconnect(test6_c2, 100L);
    assert("Good rc from disconnect", rc == MQTTCLIENT_SUCCESS, "rc was %d", rc);

    mqtt_client_destroy(&test6_c1);
    mqtt_client_destroy(&test6_c2);

exit:
    MyLog(LOGA_INFO, "%s: test %s. %d tests run, %d failures.\n",
                    (failures == 0) ? "passed" : "failed", testname, tests, failures);
    write_test_result();
    return failures;
}
#endif

int main(int argc, char** argv)
{
    int rc = 0;
    int (*tests[])() = {NULL, test1/*, test2, test3*/};
    int i;

    xml = fopen("TEST-test1.xml", "w");
    fprintf(xml, "<testsuite name=\"test1\" tests=\"%d\">\n", (int)(ARRAY_SIZE(tests) - 1));

    getopts(argc, argv);

    for (i = 0; i < options.iterations; ++i) {
        if (options.test_no == 0) { /* run all the tests */
            for (options.test_no = 1; options.test_no < ARRAY_SIZE(tests); ++options.test_no)
                rc += tests[options.test_no](options); /* return number of failures.  0 = test succeeded */
        } else
            rc = tests[options.test_no](options); /* run just the selected test */
    }

    if (rc == 0)
        MyLog(LOGA_INFO, "verdict pass");
    else
        MyLog(LOGA_INFO, "verdict fail"); 
    fprintf(xml, "</testsuite>\n");
    fclose(xml);
    return rc;
}
