/*
 * Copyright (c) 2017 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _CP_H_
#define _CP_H_

#include <pcap.h>
#include <ue.h>

#include <rte_version.h>
#include <stdbool.h>
#include <rte_ether.h>
#include <rte_ethdev.h>

#ifdef USE_REST
#include "../restoration/gstimer.h"
#endif /* USE_REST */


#ifndef PERF_TEST
/** Temp. work around for support debug log level into DP, DPDK version 16.11.4 */
#if (RTE_VER_YEAR >= 16) && (RTE_VER_MONTH >= 11)
#undef RTE_LOG_LEVEL
#define RTE_LOG_LEVEL RTE_LOG_DEBUG
#define RTE_LOG_DP RTE_LOG
#elif (RTE_VER_YEAR >= 18) && (RTE_VER_MONTH >= 02)
#undef RTE_LOG_DP_LEVEL
#define RTE_LOG_DP_LEVEL RTE_LOG_DEBUG
#endif
#else /* Work around for skip LOG statements at compile time in DP, DPDK 16.11.4 and 18.02 */
#if (RTE_VER_YEAR >= 16) && (RTE_VER_MONTH >= 11)
#undef RTE_LOG_LEVEL
#define RTE_LOG_LEVEL RTE_LOG_WARNING
#define RTE_LOG_DP_LEVEL RTE_LOG_LEVEL
#define RTE_LOG_DP RTE_LOG
#elif (RTE_VER_YEAR >= 18) && (RTE_VER_MONTH >= 02)
#undef RTE_LOG_DP_LEVEL
#define RTE_LOG_DP_LEVEL RTE_LOG_WARNING
#endif
#endif /* PERF_TEST */

#ifdef SYNC_STATS
#include <time.h>
#define DEFAULT_STATS_PATH  "./logs/"
#define STATS_HASH_SIZE     (1 << 21)
#define ACK       1
#define RESPONSE  2

typedef long long int _timer_t;

#define GET_CURRENT_TS(now)                                             \
({                                                                            \
	struct timespec ts;                                                          \
	now = clock_gettime(CLOCK_REALTIME,&ts) ?                                    \
		-1 : (((_timer_t)ts.tv_sec) * 1000000000) + ((_timer_t)ts.tv_nsec);   \
	now;                                                                         \
})

#endif /* SYNC_STATS */
/**
 * @file
 *
 * Control Plane specific declarations
 */

/*
 * Define type of Control Plane (CP)
 * SGWC - Serving GW Control Plane
 * PGWC - PDN GW Control Plane
 * SAEGWC - Combined SAEGW Control Plane
 */
enum cp_config {
	SGWC = 01,
	PGWC = 02,
	SAEGWC = 03,
};
extern enum cp_config spgw_cfg;

#ifdef SYNC_STATS
/**
 * @brief statstics struct of control plane
 */
struct sync_stats {
	uint64_t op_id;
	uint64_t session_id;
	uint64_t req_init_time;
	uint64_t ack_rcv_time;
	uint64_t resp_recv_time;
	uint64_t req_resp_diff;
	uint8_t type;
};

extern struct sync_stats stats_info;
extern _timer_t _init_time;
struct rte_hash *stats_hash;
extern uint64_t entries;
#endif /* SYNC_STATS */

/**
 * @brief core identifiers for control plane threads
 */
struct cp_params {
	unsigned stats_core_id;
	unsigned nb_core_id;
#ifdef SIMU_CP
	unsigned simu_core_id;
#endif
};

/**
 * Structure to downlink data notification ack information struct.
 */
struct downlink_data_notification {
	ue_context *context;

	gtpv2c_ie *cause_ie;
	uint8_t *delay;
	/* todo! more to implement... see table 7.2.11.2-1
	 * 'recovery: this ie shall be included if contacting the peer
	 * for the first time'
	 */
	/* */
	uint16_t dl_buff_cnt;
	uint8_t dl_buff_duration;
};

extern pcap_dumper_t *pcap_dumper;
extern pcap_t *pcap_reader;

extern int s11_fd;
extern int s11_pcap_fd;
extern int s5s8_sgwc_fd;
extern int s5s8_pgwc_fd;
extern int pfcp_sgwc_fd ;
extern struct cp_params cp_params;

#if defined(ZMQ_COMM) || defined(SDN_ODL_BUILD)
extern uint64_t op_id;
#endif /* ZMQ_COMM */
/**
 * @brief creates and sends downlink data notification according to session
 * identifier
 * @param session_id - session identifier pertaining to downlink data packets
 * arrived at data plane
 * @return
 * 0 - indicates success, failure otherwise
 */
int
ddn_by_session_id(uint64_t session_id);

/**
 * @brief initializes data plane by creating and adding default entries to
 * various tables including session, pcc, metering, etc
 */
void
initialize_tables_on_dp(void);

/**
 * Central working function of the control plane. Reads message from s11/pcap,
 * calls appropriate function to handle message, writes response
 * message (if any) to s11/pcap
 */
void
control_plane(void);

#ifdef ZMQ_COMM
/**
 * @brief Adds the current op_id to the hash table used to account for NB
 * Messages
 */
void
add_resp_op_id_hash(void);

/**
 * @brief Deletes the op_id from the hash table used to account for NB
 * Messages
 * @param nb_op_id
 * op_id received in process_resp_msg message to indicate message
 * was received and processed by the DPN
 */
void
del_resp_op_id(uint64_t resp_op_id);

#endif  /* ZMQ_COMM */

#ifdef CP_BUILD
#ifdef ZMQ_COMM
/**
 * @brief callback to handle downlink data notification messages from the
 * data plane
 * @param session id
 * session id received by control plane from the data plane
 * @return
 * 0 inicates success, error otherwise
 */
int
cb_ddn(uint64_t sess_id);

#endif  /* ZMQ_COMM */

/**
 * @brief To Downlink data notification ack of user.
 * @param dp_id
 *	table identifier.
 * @param  ddn_ack
 *	Downlink data notification ack information
 *
 * @return
 *	- 0 on success
 *	- -1 on failure
 */
int
send_ddn_ack(struct dp_id dp_id,
			struct downlink_data_notification ddn_ack);

#endif 	/* CP_BUILD */

#ifdef SYNC_STATS
/* ================================================================================= */
/**
 * @file
 * This file contains function prototypes of cp request and response
 * statstics with sync way.
 */

/**
 * Open Statstics record file.
 */
void
stats_init(void);

/**
 * Maintain stats in hash table.
 * @param sync_stats
 * sync_stats information
 *
 * @return
 * Void
 */
void
add_stats_entry(struct sync_stats *stats);

/**
 * Update the resp and ack time in hash table.
 * @param key
 * key for lookup entry in hash table
 *
 * @param type
 * Update ack_recv_time/resp_recv_time
 * @return
 * Void
 */
void
update_stats_entry(uint64_t key, uint8_t type);

/**
 * Retrive entries from stats hash table
 * @param void
 *
 * @return
 * Void
 */
void
retrive_stats_entry(void);

/**
 * Export stats reports to file.
 * @param sync_stats
 * sync_stats information
 *
 * @return
 * Void
 */
void
export_stats_report(struct sync_stats stats_info);

/**
 * Close current stats file and redirects any remaining output to stderr
 */
void
close_stats(void);
#endif   /* SYNC_STATS */
/* ================================================================================= */

/*PFCP Config file*/
#define STATIC_CP_FILE "../config/cp.cfg"

#define MAX_DP_SIZE   5
#define MAX_CP_SIZE   1
#define MAX_NUM_MME   5
#define MAX_NUM_SGWC  5
#define MAX_NUM_PGWC  5
#define MAX_NUM_SGWU  5
#define MAX_NUM_PGWU  5
#define MAX_NUM_SPGWU 5

#define SGWU_PFCP_PORT   8805
#define PGWU_PFCP_PORT   8805
#define SPGWU_PFCP_PORT   8805


typedef struct pfcp_config_t
{
       uint8_t cp_type; /*SGWC=01; PGWC=02; SAEGWC=03*/

       //MME
       uint32_t num_mme;
       struct in_addr mme_s11_ip[MAX_NUM_MME];
       uint16_t mme_s11_port[MAX_NUM_MME];

       //SGWC SAEGWC
       uint32_t num_sgwc;
       struct in_addr sgwc_s11_ip[MAX_NUM_SGWC];
       uint16_t sgwc_s11_port[MAX_NUM_SGWC];

       struct in_addr sgwc_s5s8_ip[MAX_NUM_SGWC];
       uint16_t sgwc_s5s8_port[MAX_NUM_SGWC];

       struct in_addr sgwc_pfcp_ip[MAX_NUM_SGWC];
       uint16_t sgwc_pfcp_port[MAX_NUM_SGWC];

       //PGWC
       uint32_t num_pgwc;
       struct in_addr pgwc_s5s8_ip[MAX_NUM_PGWC];
       uint16_t pgwc_s5s8_port[MAX_NUM_PGWC];

       struct in_addr pgwc_pfcp_ip[MAX_NUM_PGWC];
       uint16_t pgwc_pfcp_port[MAX_NUM_PGWC];

       //SPGWU
       uint32_t num_spgwu;
       struct in_addr spgwu_pfcp_ip[MAX_NUM_SPGWU];
       uint16_t spgwu_pfcp_port[MAX_NUM_SPGWU];

       //SGWU
       uint32_t num_sgwu;
       struct in_addr sgwu_pfcp_ip[MAX_NUM_SGWU];
       uint16_t sgwu_pfcp_port[MAX_NUM_SGWU];

       //PGWU
       uint32_t num_pgwu;
       struct in_addr pgwu_pfcp_ip[MAX_NUM_PGWU];
       uint16_t pgwu_pfcp_port[MAX_NUM_PGWU];

	//REST 	
	uint8_t transmit_cnt;
	int transmit_timer;
	int periodic_timer;

} pfcp_config_t;


void
init_pfcp(void);

void
pfcp_init_cp(void);

void
pfcp_init_s11(void);

void
pfcp_init_s5s8_pgwc(void);

void
pfcp_init_s5s8_sgwc(void);

//void
//pfcp_association_setup(void);

void get_upf_list(struct in_addr *p_upf_list);

void received_create_session_request(void);

#endif
