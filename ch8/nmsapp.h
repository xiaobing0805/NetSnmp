/************************************************************
 * Copyright (C)	GPL
 * FileName:		nmsapp.h
 * Author:		�Ŵ�ǿ
 * Date:			2014-07
 ***********************************************************/
#ifndef NMSAPP_H
#define NMSAPP_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "list.h"

#define NMSAPP_LOCKFILE "/var/run/nmsapp.pid"
#define LOCK_MODE		( S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )
#define PEER_FILE		"/usr/local/etc/nmsapp/nmshosts.conf"
#define DEFAULT_OIDS_FILE		"/usr/local/etc/nmsapp/mibs/general/monitor.oids"

#define NMS_SCAN_FREQUENCY (1)  

#ifndef MAX_OID_LEN
//#define MAX_OID_LEN       (128) //�����OID������
#endif

#define  SNMP_MSG_FF ( 0xFF )

#define USER			"user"
#define SECURITYLEVEL	"securityLevel"
#define AUTH_PASSPHRASE "auth_passphrase"
#define PRIV_PASSPHRASE "priv_passphrase"
#define FREQUENCY 		 "frequency"

//�������token�ĺ���ָ��
//typedef int (*FP_DELIME_PARSER)( char *pIn, char *pLeft, char *pRight );


typedef struct counter
{	
	int counter;             // ��ǰ����ֵ(��)
	int frequence;         // ����
} T_Counter;


typedef struct nmsoid
{
	oid oid[MAX_OID_LEN];   //MAX_OID_LEN = 128  // 1 3 6 1 2 1 1 1 0
	int oidLen;             // read_oids ��洢��ʵ�ʵ�oid ����
	int cmdType;            // ��OID ��Ӧ����������
} T_NMSOids;

typedef struct snmpv3User
{
	char	user[32];       // �û�����MD5_DES_User2
	int		secLevel;       // ��ȫ����
	char	authPass[64];
	char	privPass[64];
} T_SNMPv3User;

// ʹ��ϵͳĬ�ϵ�UDPЭ���
typedef struct
{
	char				hostName[32];   // ip
	T_SNMPv3User		snmpv3User;
	struct snmp_session *pSess;         // SNMP �Ự��Ϣ:ÿ��������Ӧһ���Ự��Ϣ,
	T_List				*pList;         // ��������Ϊ T_NMSOids
	// ��ʹ�ûص�����ʱ����һ��OID�ڵ�ʱ��ptCurrentOid = ptCurrentOid->next;
	T_ListNode *ptCurrentOid;           // ָ�� T_NMSOids ����ʾ��ǰ���͵�OID
	T_Counter  tCounter;
} T_NMSHost;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /*  NMSAPP_H  */
