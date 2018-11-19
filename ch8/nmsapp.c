/************************************************************
 * Copyright (C)	GPL
 * FileName:		nmsapp.c
 * Author:		�Ŵ�ǿ
 * Date:			2014-07
 * ����ϵͳ��centos6.5/fedora12
 * gcc �汾��gcc version 4.4.7 20120313
 ***********************************************************/
#include "nmsapp.h"
#include "defines.h"
#include "list.h"
#include "tools.h"

static int check_nmsapp_running( void );


static void save_peer_oids( T_List *pList, char *pcOid );


static int read_monitor_oids( void* data, void* ctx );


static int read_oids( void );


static int compare_ip_string( void* data, void* ctx );


static int init_snmpv3AP_sess( void* data, void* ctx );

//static int synch_send( netsnmp_session *pSs, oid* pOid, int len );

static int read_oids2List(char *fN, T_List      *pList);

static struct snmp_pdu *create_pdu_withNULL(int cmdTp,oid*pOid,int oidLen );

// ϵͳ��������
static T_List  *pMainNMSList   = NULL;
//��ǰ���ڴ����������(�Ự�Ļص��������Լ�)
static int    activeHosts             = 0; 
// ϵͳ����
static int running = 1;
static int timerId;

/***********************************************************
* Description:    �������ʼ��
***********************************************************/
T_List *init_NMS_list( void )
{
	pMainNMSList = create_list( );
	return pMainNMSList;
}


/***********************************************************
* Description:   ���أ�TRUE-�������У�FALSE-δ����
***********************************************************/
static int check_nmsapp_running( )
{
	int iLkFd;
	char buff[32];

	// ��д��ʽ�����ļ����û����û�����ж�дȨ�ޣ������û����пɶ�Ȩ��
	iLkFd = open( NMSAPP_LOCKFILE, O_RDWR | O_CREAT, LOCK_MODE );
	if( iLkFd < 0 )
	{
		printf( "check_nmsapp_running:can not open %s:%s", NMSAPP_LOCKFILE, strerror( errno ) );
		exit( 1 );
	}

	if( FAILURE == lock_file( iLkFd ) )
	{
		if( ( errno == EAGAIN ) || ( errno == EACCES ) )
		{
			close( iLkFd );
			return TRUE;
		}
		printf( "check_nmsapp_running:can not lock %s:%s", NMSAPP_LOCKFILE, strerror( errno ) );
		exit( 1 );
	}

	// ��������С��ȡΪ0
	ftruncate( iLkFd, 0 );
	// д����̺ŵ��ļ�
	sprintf( buff, "%d", getpid( ) );
	write( iLkFd, buff, strlen( buff ) );

	return FALSE;
}


/***********************************************************
* Description:  p1, p2 ��Ϊ��
* p1: 192.168.43.146:161��p2: 192.168.43.146 ��������ͬ��IP
* �ַ��� ��ͬ���� FAILURE�����򷵻� SUCCESS
***********************************************************/
static int compare_ip_string( void* data, void* ctx )
{
	char pLeft1[128] = { 0 };
	char pLeft2[128] = { 0 };

	char            * p1    = (char*)ctx;
	T_NMSHost       * p2    = (T_NMSHost*)data;

	if( NULL != p1 && NULL != p2 )
	{
		get_token_ip( p1, pLeft1 );
		get_token_ip( (char*)p2->hostName, pLeft2 );
		//printf("cmp: %s,%s;p1=%s,p2=%s\n",pLeft1,pLeft2,p1,p2);
		if( 0 == strcmp( pLeft1, pLeft2 ) )
		{
			return FAILURE;
		} else
		{
			return SUCCESS;
		}
	}

	return FAILURE;
}

/***********************************************************
* Description:    ����T_NMSHost �ڵ㣬������ӵ�������
***********************************************************/
static T_NMSHost * create_NMSHost_node( void )
{
	// ����ڵ�ռ�,ʧ��ʱ���Ը����������������
	T_NMSHost *pNode = MALLOC_TYPEDEF_STRUCT( T_NMSHost );

	if( NULL != pNode )
	{
		// �����ýṹ���е�������
		pNode->pList = create_list( );
		if( NULL == pNode->pList )
		{
			NMSAPP_FREE( pNode );
			return NULL;
		}
		pNode->ptCurrentOid = NULL;
		pNode->tCounter.counter = 0;
		pNode->tCounter.frequence = 1; //��ʼ������Ϊ1��
		return pNode;
	}else
	{
		return NULL;
	}
}

/***********************************************************
* Description:    ������ص����������ļ���
* ֻ֧�����ָ�ʽ��IP��ַ
***********************************************************/
static int read_nmsapp_conf( )
{
	FILE            *pf = NULL;
	/* ע�⣺ֻʹ����128���ַ����� */
	char acLine[128] = { 0 };
	char acT[128]        = { 0 };
	char acV[128]        = { 0 };
	int jj                      = -1;
	int readflag        = 1;
	T_NMSHost       *pHostNode      = NULL; //�ڵ���뵽������ pMainNMSList ��

	if( NULL != ( pf = fopen( PEER_FILE, "r" ) ) )
	{
		// ѭ����ȡ��ÿ�ζ�ȡһ��:fget
		while( fgets( acLine, sizeof( acLine ) - 1, pf ) )
		{
			// ����:bzero��������
			bzero( acT, (size_t)sizeof( acT ) );
			bzero( acV, (size_t)sizeof( acV ) );

			//���˲�����[]=����,����У�ע�͵�
			// ���� [192.168.43.132:1611]
			if( NULL == strstr( acLine, LEFT_SQUARE_BRACKET )
			    && NULL == strstr( acLine, EQUAL_SIGN )
			    )
			{
				bzero( acLine, (size_t)sizeof( acLine ) );
				continue;
			}

			// ����������
			if( NULL != strstr( acLine, LEFT_SQUARE_BRACKET )
			    && SUCCESS == parser_delim( acLine, LEFT_SQUARE_BRACKET, acT, acV ) )
			{
				//printf("t1:%s,v1:%s\n",(acT!=NULL)?acT:"(null)",acV);
				if( SUCCESS == parser_delim( acV, RIGHT_SQUARE_BRACKET, acT, acV ) )
				{
					//���洢��һ�������ɹ��Ľڵ㵽��������
					if( NULL != pHostNode )
					{
						list_append( pMainNMSList, (void*)pHostNode, NULL );
						pHostNode = NULL;
					}

					// �����Ƿ�����ͬ�� hostname,FAILURE ��ʾ����ͬ�ģ�
					//����Ժ�������������ֱ���µ�hostname
					if( FAILURE == list_foreach( pMainNMSList, compare_ip_string, (void*)acT ) )
					{
						printf( "--%s is duplicated\n", acT );
						readflag = 0; //����Ч��ʶ
					}else
					{
						// ��ʼ���ڵ�
						pHostNode = create_NMSHost_node( );
						if( NULL == pHostNode )
						{
							continue;
						}

						strcpy( pHostNode->hostName, acT );
						readflag = 1;
					}
				}
			}else if( readflag )
			{
				/// �ж϶�ȡ�ͽ���name = value �Ƿ�ɹ�
				if( FAILURE == parser_delim( acLine, EQUAL_SIGN, acT, acV ) )
				{
					continue;
				}

				if( 0 == strcmp( acT, USER ) )
				{
					strcpy( pHostNode->snmpv3User.user, acV );
				} else if( 0 == strcmp( acT, SECURITYLEVEL ) )
				{
					pHostNode->snmpv3User.secLevel = atoi( acV );
				} else if( 0 == strcmp( acT, AUTH_PASSPHRASE ) )
				{
					strcpy( pHostNode->snmpv3User.authPass, acV );
				} else if( 0 == strcmp( acT, PRIV_PASSPHRASE ) )
				{
					strcpy( pHostNode->snmpv3User.privPass, acV );
				}else if( 0 == strcmp( acT, FREQUENCY ) )
				{
					pHostNode->tCounter.frequence = atoi( acV );
				}
			}
		}

		//���洢��һ�������ɹ��Ľڵ㵽��������
		if( NULL != pHostNode )
		{
			list_append( pMainNMSList, (void*)pHostNode, NULL );
			pHostNode = NULL;
		}

		fclose( pf );
		return SUCCESS;
	}else
	{
		printf( "read_peer_conf:fopen '%s' is failed \n", PEER_FILE );
		return FAILURE;
	}
}

/*
   �����ľ��� SNMP_MSG_GETNEXT �� SNMP_MSG_GETBULK ���ο�Դ�� snmpwalk.c
   �����ر��벢����ѧϰ��
   ����һ�£�������ü򵥵� SNMP_MSG_GETNEXT
 */


/***********************************************************
* Description:    ����ȡ������ת��ΪOID
* Ȼ�����ÿ��OID���ص㣺
* ĩβ��0�ľ��� SNMP_MSG_GET PDU�����ĩβ�Ƿ��.0
***********************************************************/
static void save_peer_oids( T_List *pList, char *pcOid )
{
	char            *ptmp = NULL;
	// ��Ϊ�����һ���ڵ�
	T_NMSOids       *pNMSOid = MALLOC_TYPEDEF_STRUCT( T_NMSOids );

	if( NULL != pList && NULL != pcOid && NULL != pNMSOid )
	{
		//ȥ���ո�
		ptmp = trim_ends_space( pcOid );
		pNMSOid->oidLen = sizeof( pNMSOid->oid ) / sizeof( pNMSOid->oid[0] );

		/* read_objid
		   MIB search path: $HOME/.snmp/mibs:/usr/local/share/snmp/mibs
		   snmp_parse_oid
		   get_node
		 */
		if( !read_objid( ptmp, pNMSOid->oid, (size_t*)&pNMSOid->oidLen ) )
		{
			snmp_perror( "-- snmp_perror: read_objid" );
			printf( "--read oid:%s occer error!! ignore it and free\n", ptmp );
			NMSAPP_FREE( pNMSOid );
			return;
		}

		// �ж������λ�Ƿ�Ϊ.0
		if( '.' == ptmp[strlen( ptmp ) - 2]
		    && '0' == ptmp[strlen( ptmp ) - 1]
		    )
		{
			pNMSOid->cmdType = SNMP_MSG_GET;
		}else
		{
			//û��.0 �ľͰ����������
			printf( "--%s:there is no .0,how to resolve it !\n", ptmp );
			pNMSOid->cmdType = SNMP_MSG_GET;
		}

		list_append( pList, pNMSOid, NULL );
	}

	return;
}


static int read_oids2List(char *fN, T_List      *pList)
{
	char acLine[128]             = { 0 };
	FILE            *pf                             = NULL;
	if(NULL != pList && NULL != fN)
	{
		if( NULL != ( pf = fopen( fN, "r" ) ) )
		{
			while( fgets( acLine, sizeof( acLine ) - 1, pf ) )
			{
				// ��OID������;���ǿ���
				if( acLine[0] != '\n' && acLine[0] != '\r' && acLine[0] != '\0' )
				{
					save_peer_oids( pList, acLine );
				}
			}

			fclose( pf );
			pf = NULL;
			return SUCCESS;

		}else
		{
			printf( "read_nmsapp_oids:fopen '%s' is failed \n", fN );
			return FAILURE;
		}
	}
	return FAILURE;
}

/***********************************************************
* Description:    ��ȡ����������������ļ�
* �ú��� �����ctxΪNULL
***********************************************************/
static int read_monitor_oids( void* data, void* ctx )
{
	char ip[128]  = { 0 };
	char fn[128] = { 0 };
	T_NMSHost       *pM = (T_NMSHost*)data;
	int ret = FAILURE;

	if( NULL == pM )
		return FAILURE;

	//   /usr/local/etc/nmsapp/mibs/192.168.43.146/monitor.oids
	snprintf( fn, strlen( fn ) - 1,
	          "/usr/local/etc/nmsapp/mibs/%s/monitor.oids",
	          get_token_ip( (char*)pM->hostName, ip ) );

	printf( "--open FileName to read oids:%s\n\n", fn );

	ret = read_oids2List( fn, pM->pList);
	ret = read_oids2List( DEFAULT_OIDS_FILE, pM->pList);
	return ret;
}


/***********************************************************
* Description:   ��ȡOID
***********************************************************/
static int read_oids( void )
{
	return list_foreach( pMainNMSList, read_monitor_oids, NULL );
}

/***********************************************************
* Description:    ��ӡOID
***********************************************************/
static int _print_nmsoids( void* data, void* ctx )
{
	T_NMSOids       *pOids  = (T_NMSOids*)data;
	int ii              = 0;
	if( NULL != pOids )
	{
		while( ii < pOids->oidLen )
		{
			printf( "%d", pOids->oid[ii] );
			if( ii < pOids->oidLen - 1 )
			{
				printf( "." );
			}

			ii++;
		}

		printf( "(cmd:%X)\n", pOids->cmdType );
		return SUCCESS;
	}
	return FAILURE;
}

/***********************************************************
* Description:    ��ӡ����(bit4) 1 1  1 1(bit1)
***********************************************************/
static int _print_nms_list( void* data, void* ctx )
{
	T_NMSHost       *pNMSHost       = (T_NMSHost*)data;
	int flag            = (int)ctx;
	printf("---------_print_nms_list:%d",flag);
	if( NULL != pNMSHost )
	{
		printf( "|--------------\n%s\n", pNMSHost->hostName );

		// bit:1
		if( flag & 0x01 )
		{
			printf( "snmpv3User:user=%s\n", pNMSHost->snmpv3User.user );
			printf( "secLevel=%d\n", pNMSHost->snmpv3User.secLevel );
			printf( "authPass=%s\n", pNMSHost->snmpv3User.authPass );
			printf( "privPass=%s\n", pNMSHost->snmpv3User.privPass );
		}
		// bit:2
		if( NULL != pNMSHost->pSess && ( flag & 0x02 ) )
		{
			printf( "version:%d\n", pNMSHost->pSess->version );
			printf( "peername:%s\n", pNMSHost->pSess->peername );
			printf( "securityName:%s\n", pNMSHost->pSess->securityName );
			printf( "......\n" );
		}
		// bit:3
		if( NULL != pNMSHost->pList && ( flag & 0x04 ) ) //��ӡ T_NMSOids ����
		{
			list_foreach( pNMSHost->pList, _print_nmsoids, NULL );
		}
		// bit:4
		if( NULL != pNMSHost->ptCurrentOid && ( flag & 0x08 ) )
		{
			T_NMSOids* pOids = (T_NMSOids*)pNMSHost->ptCurrentOid;
			printf( "\nptCurrentOid:\n" );
			_print_nmsoids( pNMSHost->ptCurrentOid, NULL );
		}

		printf( "--------------|\n" );
		return SUCCESS;
	}
	return FAILURE;
}

/***********************************************************
* Description:    ��ӡ����(bit4) 1 1  1 1(bit1)
***********************************************************/
static void print_nms_main_list( int flag )
{
	printf( "=====main list size:%d=====\n", pMainNMSList->size );
	list_foreach( pMainNMSList, _print_nms_list, (void*)flag );
	return;
}

/***********************************************************
* Description:   ��ӡsnmp��ȡ�Ľ��
***********************************************************/
int print_result( int status, struct snmp_session *sp, struct snmp_pdu *pdu )
{
	char buf[1024];
	struct variable_list    *vp;
	int ix;
	struct timeval now;
	struct timezone tz;
	struct tm                               *tm;

	gettimeofday( &now, &tz );
	tm = localtime( &now.tv_sec );
	fprintf( stdout, "%.2d:%.2d:%.2d.%.6d ", tm->tm_hour, tm->tm_min, tm->tm_sec,
	         now.tv_usec );
	switch( status )
	{
	case STAT_SUCCESS:
		vp = pdu->variables;
		if( pdu->errstat == SNMP_ERR_NOERROR )
		{
			while( vp )
			{
				snprint_variable( buf, sizeof( buf ), vp->name, vp->name_length, vp );
				fprintf( stdout, "%s: %s\n", sp->peername, buf );
				vp = vp->next_variable;
			}
		}else
		{
			for( ix = 1; vp && ix != pdu->errindex; vp = vp->next_variable, ix++ )
			{
				;
			}
			if( vp )
			{
				snprint_objid( buf, sizeof( buf ), vp->name, vp->name_length );
			} else
			{
				strcpy( buf, "(none)" );
			}
			fprintf( stdout, "%s: %s: %s\n",
			         sp->peername, buf, snmp_errstring( pdu->errstat ) );
		}
		return SUCCESS;
	case STAT_TIMEOUT:
		fprintf( stdout, "%s: Timeout\n", sp->peername );
		return FAILURE;
	case STAT_ERROR:
		snmp_perror( sp->peername );
		return FAILURE;
	}
	return FAILURE;
}


/***********************************************************
* Description:  ������Ӧ�Ļص�����;magic Ϊ�û����������
   ��Զ����������Ӧʱ���ûص�������
   �ص��������Զ������ɸûỰ�������(snmp_read�²�⺯������)��
   ������Ӧ�Ľ����
   ������һ��OID������
   ����PDU�����ͣ���������ڵĻ���
***********************************************************/
int asynch_response_cb( int operation, struct snmp_session *sp, int reqid,
                        struct snmp_pdu *pdu, void *magic )
{
	T_NMSHost* pNMShost = (T_NMSHost*)magic;
	T_NMSOids* pNMSOids = NULL;
	struct snmp_pdu *req;

	if( NULL != pNMShost && NULL != pNMShost->ptCurrentOid)
		if( operation == NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE )
		{
			// ��������
			if( SUCCESS == print_result( STAT_SUCCESS, pNMShost->pSess, pdu ) )
			{
				// ��һ��IOD
				pNMShost->ptCurrentOid = pNMShost->ptCurrentOid->next;
				if( pNMShost->ptCurrentOid )
				{
					pNMSOids = (T_NMSOids*)pNMShost->ptCurrentOid->data;
					req = create_pdu_withNULL(pNMSOids->cmdType,pNMSOids->oid, pNMSOids->oidLen );
					if( NULL != req )
					{
						if( snmp_send( pNMShost->pSess, req ) )
						{
							return 1;
						} else
						{
							//�������ʧ�ܣ���¼������Ϣ��ͬʱ�ͷ�pdu
							snmp_perror( "snmp_send" );
							snmp_free_pdu( req );
						}
					}
				}
			}
		}else // ����������Գ�ʱ������ӡ��ǰ�ĻỰ��Ϣ
		{
			print_result( STAT_TIMEOUT, pNMShost->pSess, pdu );
		}
	/*
	   �����ִ������Ѵ�������������е�����ʱ��
	   û�м������Ͳ�ѯPDUʱ
	 */
	activeHosts--;
	return 1;
}

/***********************************************************
* Description:  ����snmpv3�Ự�������Ự��
   �����������������Ҫ�رմ򿪵ĻỰ���ͷ���Դ
   �˺��������سɹ�����ʹ�ÿ��Գ�ʼ�����лỰ
***********************************************************/
static int init_snmpv3AP_sess( void * data, void * ctx )
{
	T_NMSHost                       * pNMSHost = (T_NMSHost* )data;
	struct snmp_session sess;
	snmp_sess_init( &sess );

	sess.version                    = SNMP_VERSION_3; //SNMP_VERSION_2c ,SNMP_VERSION_1
	sess.peername                   = strdup( pNMSHost->hostName );
	sess.securityName               = strdup( pNMSHost->snmpv3User.user );
	sess.securityNameLen    = strlen( sess.securityName );
	sess.securityLevel              = pNMSHost->snmpv3User.secLevel;

	// ʹ�ûص����� :�ص��������ڴ��������󷵻غ��¸�����PDU����
	sess.callback           = asynch_response_cb;
	sess.callback_magic = pNMSHost;

	// ��������֤,����֤:MD5
	if( SNMP_SEC_LEVEL_AUTHNOPRIV <= sess.securityLevel )
	{
		sess.securityAuthProto          = usmHMACMD5AuthProtocol;
		sess.securityAuthProtoLen       = USM_AUTH_PROTO_MD5_LEN;
		sess.securityAuthKeyLen         = USM_AUTH_KU_LEN;

		if( generate_Ku( sess.securityAuthProto, sess.securityAuthProtoLen,
		                 (u_char* )pNMSHost->snmpv3User.authPass, strlen( pNMSHost->snmpv3User.authPass ),
		                 sess.securityAuthKey, &sess.securityAuthKeyLen ) != SNMPERR_SUCCESS )
		{
			snmp_perror( "init_snmpv3AP_sess" );
			printf( "Error generating Ku from authentication pass phrase for %s \n", pNMSHost->pSess->peername );
			//NMSAPP_FREE(pNMSHost->pSess);
			//return FAILURE;
		}
	}

	// ����֤�ͼ���,�ü���Э��: DES
	if( SNMP_SEC_LEVEL_AUTHPRIV <= sess.securityLevel )
	{
		// �������Э�� USM_PRIV_KU_LEN 32
		sess.securityPrivProto          = usmDESPrivProtocol;
		sess.securityPrivProtoLen       = USM_PRIV_PROTO_DES_LEN; //10=sizeof(usmDESPrivProtocol)/sizeof(oid);
		sess.securityPrivKeyLen         = USM_PRIV_KU_LEN;
		
		if( generate_Ku( sess.securityAuthProto, sess.securityAuthProtoLen,
		                 (u_char* )pNMSHost->snmpv3User.privPass, strlen( pNMSHost->snmpv3User.privPass ),
		                 sess.securityPrivKey, &sess.securityPrivKeyLen ) != SNMPERR_SUCCESS )
		{
			snmp_perror( "init_snmpv3AP_sess" );
			printf( "Error generating Ku from privacy pass phrase for %s \n", sess.peername );
		}
	}

	pNMSHost->pSess = snmp_open( &sess );
	// ����û�гɹ��򿪵ĻỰ���˴�û�д����������������ķ���ʱͳһ����
	if( !pNMSHost->pSess )
	{
		snmp_sess_perror( "init_snmpv3AP_sess:snmp_open", &sess ); //��¼������Ϣ����־�У�
		printf( "snmp_open failed!!\n\n" );
	}
	return SUCCESS;
}

/***********************************************************
* Description:    ����snmpv3�Ự�������Ự
***********************************************************/
void init_snmpv3_sesssions( void )
{
	list_foreach( pMainNMSList, init_snmpv3AP_sess, NULL );
	return;
}

/***********************************************************
* Description:     ��ʼ������Ҫ���͵�OID��Ϊ�����ͷ���ڵ�
***********************************************************/
static int _init_send_oids( void* data, void* ctx )
{
	T_NMSHost *pHostNode = (T_NMSHost*)data;
	T_ListNode *pHead = NULL;
	if( NULL != pHostNode && NULL != pHostNode->pList )
	{
		if( NULL == pHostNode->ptCurrentOid
		    && pHostNode->tCounter.counter++ >= pHostNode->tCounter.frequence
		    )     // ��δ�������ʱ�������г�ʼ��
		{
			pHostNode->ptCurrentOid = pHostNode->pList->head;
			pHostNode->tCounter.counter = 1;
		}
		return SUCCESS;
	}
	return FAILURE;
}

/***********************************************************
* Description:     ��ʼ������Ҫ���͵�OID��Ϊ�����ͷ���ڵ�
***********************************************************/
static void init_send_oids( void )
{
	list_foreach( pMainNMSList, _init_send_oids, NULL );
	return;
}


static struct snmp_pdu *create_pdu_withNULL(int cmdTp,oid*pOid,int oidLen )
{
	if( NULL != pOid && oidLen > 0)
	{
		struct snmp_pdu *pdu = snmp_pdu_create( cmdTp );
		snmp_add_null_var( pdu, pOid,oidLen );
		return pdu;
	}
	return NULL;
}

/***********************************************************
* Description:    ��������,ÿ������ִֻ��һ��
***********************************************************/
static int _send_request( void* data, void* ctx )
{
	T_NMSHost               *pNMSHost       = (T_NMSHost*)data;
	T_NMSOids               *pOidNode       = NULL;
	int n                       = 0;
	// ��ÿһ����Ч�ĻỰ������PDU������������
	struct snmp_pdu *pdu;
	if( NULL != pNMSHost
	    && NULL != pNMSHost->pSess
	    && NULL != pNMSHost->ptCurrentOid
	    )
	{
		// ȡ��ǰ�����͵�OID
		pOidNode = (T_NMSOids*)pNMSHost->ptCurrentOid->data;
		if( NULL != pOidNode // ͷ�ڵ�ʱ�ŷ���
		    && pNMSHost->ptCurrentOid ==  pNMSHost->pList->head )
		{
			pdu = create_pdu_withNULL(pOidNode->cmdType, pOidNode->oid, pOidNode->oidLen );
			if( NULL != pdu )
			{
				if( snmp_send( pNMSHost->pSess, pdu ) )
				{ 
				/*
				ÿ������һ�����������������Ѿ������������;
				���ɹ�����PDU������������
				����������ʾ��Ҫ���м������������Ӧ��������
				*/
					activeHosts++;
				}else
				{
					snmp_perror( "snmp_send" );
					printf( "--snmp_send failed\n\n" );
					snmp_free_pdu( pdu );
				}
			}
		}
	}
}

/***********************************************************
* Description:    �첽�ķ�ʽ:ÿ�η���һ�������е�һ��OID
***********************************************************/
static void asynch_send( void )
{
	/*
	ѭ���������еļ�������� ÿ����������һ����ǰ��OID;
	��ÿ�����󴴽��Ự������PDU�������ͣ�
	*/
	list_foreach( pMainNMSList, _send_request, NULL );
	printf( "we hava sended %d hosts~~\n\n", activeHosts );
	return;
}

/***********************************************************
* Description:    �ȴ��������������
***********************************************************/
static int wait_request( void )
{
	/*
	�������ȴ���activeHosts�ɻص������Զ��Լ�
	Ϊ0ʱwhile �������������������е��������󶼴������
	*/
	while( activeHosts )
	{
		int fds = 0, block = 1;
		fd_set fdset;
		struct timeval timeout;
		FD_ZERO( &fdset );          //��������
		// �ú����ܹ���ȡ��ע��������
		snmp_select_info( &fds, &fdset, &timeout, &block );
		fds = select( fds, &fdset, NULL, NULL, block ? NULL : &timeout );
		if( fds < 0 )
		{
			perror( "select failed" );
			exit( 1 );
		}
		if( fds )
		{
			snmp_read( &fdset );    // ��ȡ���е�socket
		}else
		{
			snmp_timeout( );
		}
	}
	return 0;
}

/***********************************************************
* Description:   �رջỰ���ͷ���Դ
***********************************************************/
static int _close_sessions( void* data, void* ctx )
{
	T_NMSHost *pNMSHost = (T_NMSHost*)data;
	if( NULL != pNMSHost && NULL != pNMSHost->pSess )
	{
		snmp_close( pNMSHost->pSess );
	}
	return SUCCESS;
}

/***********************************************************
* Description:     �رջỰ���ͷ���Դ
***********************************************************/
static void close_sessions( void )
{
	list_foreach( pMainNMSList, _close_sessions, NULL );
	return;
}

static int get_main_list_size()
{
	return (int)list_size( pMainNMSList);
}


static int _get_oid_list_size(void *data, void *ctx)
{
	T_NMSHost       *pNMSHost       = (T_NMSHost*)data;
	static int size  = 0;
	if( 0 !=        *(int*)ctx )
		size = *(int*)ctx;
	else size = 0;

	if( NULL != pNMSHost && NULL != pNMSHost->pList)
	{
		size += list_size(pNMSHost->pList);
	}
	*(int*)ctx = size;

	return SUCCESS;
}


static int get_oid_list_size()
{
	int size = 0;
	list_foreach( pMainNMSList, _get_oid_list_size,  (void*)&size);
	return size;
}


static void loop_send_get(unsigned int clientreg, void *clientarg )
{
	init_send_oids( );
	asynch_send( );
	wait_request( );
}


void quitNMS( int sigNo)
{
	
	if( 0 < timerId)
	{
		snmp_alarm_unregister(timerId);
	}

	printf("\n\nI am quit now ~~\n\n");
	running  = 0;
	return ;
}


/***********************************************************
* Description:   ��������������
***********************************************************/
int main( void )
{
	if( NULL == init_NMS_list( ) )
	{
		return 1;
	}

	if( check_nmsapp_running( ) )
	{
		printf( "snmpapp:is running\n" );
		return 1;
	}
	printf( "==========read_nmsapp_conf==============\n" );
	if( FAILURE == read_nmsapp_conf( PEER_FILE )  ||( 0 == get_main_list_size()))
	{
		printf( "snmsapp:read_peer_conf failed,exit!!\n" );
		return 1;
	}

	//print_nms_main_list(0x0f);

	printf( "==========read_nmsapp_oids==============\n" );

	// ֻ�� init_snmp ����ܽ�����ص�MIB����!!!
	// ��read_objid
	init_snmp( "nmsapp" );

	if( FAILURE == read_oids( ) || (0 == get_oid_list_size()))
	{
		printf( "snmsapp:read_nmsapp_oids failed once,exit!!\n" );
		return 1;
	}

	//print_nms_main_list(0x01);

	init_snmpv3_sesssions( );

	//print_nms_main_list(0x0f);

	//signal(SIGINT, &quitNMS);
	while(running)
	{

		init_send_oids( );
		// ��ӡ���ļ�fprintf(f, ".%s(%lu)", tp->label, tp->subid);
		asynch_send( );
		wait_request( );
		sleep( 1 );//usleep
	}
       printf("come here........\n");
	return 0;
}




