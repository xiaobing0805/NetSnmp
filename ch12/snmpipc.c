/************************************************************
 * Copyright (C)	GPL
 * FileName:		snmpipc.c
 * Author:		�Ŵ�ǿ
 * Date:			2014-08
 * gcc -g -Wall -fPIC -shared snmpipc.c -o libsnmpipc.so
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include "snmpipc.h"

int update_shm_data( int dType, 
T_ShmCellVal *pValue, int diretion );

// union semun {
// int val; /* value for SETVAL */
// struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
// unsigned short *array; /* array for GETALL, SETALL */
// struct seminfo *__buf; /* buffer for IPC_INFO */
// };

// �ź���ID
static int s_SemId;
static int s_ShmId;


// �����ֹ�ά��,�ṹ���ӳ���
static T_MapTable s_tParaMapTable[] =
{
	{ .iNo = PARA_A, .iOffset = offsetof( T_ParaData, a ),					  .iLen = sizeof( int ) },
	{ .iNo = PARA_B, .iOffset = offsetof( T_ParaData, b ),					  .iLen = MAX_CHAR_LEN	},
	{ .iNo = PARA_C1, .iOffset = offsetof( T_ParaData, c ),					  .iLen = sizeof( int ) },
	{ .iNo = PARA_C2, .iOffset = offsetof( T_ParaData, c ) + sizeof( int ),	  .iLen = sizeof( int ) },
	{ .iNo = PARA_C3, .iOffset = offsetof( T_ParaData, c ) + 2 * sizeof( int ), .iLen = sizeof( int ) },
};
#define PARA_NUM ( sizeof( s_tParaMapTable ) / sizeof( T_MapTable ) )

static T_MapTable s_tRealDtMapTable[] =
{
	{ .iNo = XY0X, .iOffset = offsetof( T_RealData, xy[0].x ),	.iLen = sizeof( int ) },
	{ .iNo = XY0Y, .iOffset = offsetof( T_RealData, xy[0].y ),	 .iLen = sizeof( int ) },
	{ .iNo = XY1X, .iOffset = offsetof( T_RealData, xy[1].x ) , .iLen = sizeof( int ) },
	{ .iNo = XY1Y, .iOffset = offsetof( T_RealData, xy[1].y ) , .iLen = sizeof( int ) },
	{ .iNo = REALZ, .iOffset = offsetof( T_RealData, z ), .iLen = sizeof( int ) },
};
#define REALDATA_NUM ( sizeof( s_tRealDtMapTable ) / sizeof( T_MapTable ) )

static T_MapTable s_tAlarmDtMapTable[] =
{
	{ .iNo = ALARM1, .iOffset = offsetof( T_AlarmData, alarm1 ),	.iLen = sizeof( int ) },
	{ .iNo = ALARM2, .iOffset = offsetof( T_AlarmData, alarm2 ),	 .iLen =  MAX_CHAR_LEN },
	{ .iNo = ALARM_COUNTER, .iOffset = offsetof( T_AlarmData, alarmCounter ) , .iLen = sizeof( int ) },
};
#define ALARMDATA_NUM ( sizeof( s_tAlarmDtMapTable ) / sizeof( T_MapTable ) )


// ��������С����
static int s_acMaxObjNum[SHM_TYPE_NUM] =
{
	PARA_NUM,
	REALDATA_NUM,
	ALARMDATA_NUM,
	0,
};

// �����ڴ��С���ַ
static T_ShareMem s_atShareMem[] =
{
	{ .iSize = PARA_NUM * sizeof( T_ShmCellVal ), .pShmAddr = NULL },
	{ .iSize = REALDATA_NUM * sizeof( int ),	  .pShmAddr = NULL },
	{ .iSize = ALARMDATA_NUM * sizeof( int ),	  .pShmAddr = NULL },
};

#define SHM_ARRAY_SIZE	sizeof( s_atShareMem ) / sizeof( T_ShareMem )
#define SHM_CELL_SIZE	sizeof( T_ShmCellVal )

static void check_file_exist( char *pName );
static int get_maxobj_num(int dType);
static void update_para_data(T_ParaData* pData, int direction);
static void update_realtime_data(T_RealData* pData, int direction);
/***********************************************************
* Description:    ����dType���������ж��������
***********************************************************/
static int get_maxobj_num(int dType)
{
	if( ( dType >= SHM_PARADATA) 
			&& (dType < SHM_TYPE_NUM ) 
	   )
		return s_acMaxObjNum[dType];
		
    return FAILURE;
}

/***********************************************************
* Description:    ��ȡdType��������MAP�ڴ����ʼ��ַ
***********************************************************/
T_MapTable* get_maptable(int dType)
{
	T_MapTable * pMapData =  NULL;
	if( dType == SHM_PARADATA )
	{
		pMapData = s_tParaMapTable;
		
	}else if( dType == SHM_REALDATA )
	{
		pMapData = s_tRealDtMapTable;
	}else if( dType == SHM_ALARM )
	{
		pMapData = s_tAlarmDtMapTable;
	}
	return pMapData;
}

/***********************************************************
* Description:    ���ؿ����ĳ���
***********************************************************/
static int app_memcpy( void* pValInner, void* pShmVal, int iLen, int diretion )
{
	if( NULL == pValInner || NULL == pShmVal || iLen <= 0 )
		return 0;

	if( TO_SHM == diretion )            // ���õ������ڴ�
	{
		memcpy( pShmVal, pValInner, iLen );
	} else if( FROM_SHM == diretion )   // �ӹ����ڴ�ȡ
	{
		memcpy( pValInner, pShmVal, iLen );
	} else
	{
		return 0;
	}
	return iLen;
}

/***********************************************************
* Description:    ����ļ��Ƿ���ڣ��������򴴽�
***********************************************************/
static void check_file_exist( char *pName )
{
	FILE *pF;
	if( access( pName, F_OK ) < 0 )
	{
		pF = fopen( pName, "w" );
		if( pF )
		{
			///����д��·�������ļ���
			fwrite( pName, strlen( pName ), 1, pF );
			fclose( pF );
		}else
		{
			printf( "check_file_exist: %s  failed!!", pName );
		}
	}
	return;
}

/***********************************************************
* Description:  
   bIsMasterΪ�洴�������ڴ棬�����ȡ�����ڴ��ID��
   ��Ϊ�����⣬�ú���ֻҪ��ҵ����̵���һ�ξ�OK�ˣ�
   �������bIsMasterΪ1��SNMP����ɲ��õ��øýӿڣ�   
***********************************************************/
static int init_share_memory( BOOLEAN bIsMaster ) 
{
	int		i;
	char	acFile[256] = { 0 };
	int		iShmSize	= 0;
	void	*pShmAddr	= NULL;
	int		iShmKey;//, iShmId;

	// ���㹲���ڴ��ܴ�С
	for( i = 0; i < SHM_ARRAY_SIZE; i++ )
		iShmSize += iShmSize + s_atShareMem[i].iSize;

	if( iShmSize <= 0 )		return FAILURE;

	// ���ļ����ڻ�ȡ�����ڴ��key
	sprintf( acFile, "%s%s", APP_DIR, SHM_CONF );
	check_file_exist( acFile );
	// ���keyֵ
	iShmKey = ftok( acFile, SHM_KEY_ID );
	if( iShmKey == (key_t)-1 )
	{
		printf( "get_share_memory:ftok() for shm failed!!\n" );
		return FAILURE;
	}
	printf( "ftok return key = 0x%x\n", iShmKey );

	if( bIsMaster )
	{
		// ���������û��ɶ�д�Ĺ����ڴ�
		s_ShmId = shmget( iShmKey, iShmSize, 0666 | IPC_CREAT );
		if( -1 == s_ShmId )
		{
			printf( "get_share_memory:shmget() failed!!\n" );
			return FAILURE;
		}
	}else
	{
		// ��ȡ�����ڴ�
		s_ShmId = shmget( iShmKey, iShmSize, 0666 );
		if( -1 == s_ShmId )
		{
			printf( "get_share_memory:shmget() failed!!\n" );
			return FAILURE;
		}
	}

	// ���һ������Ϊ0 ��ʾ�ɶ�д
	pShmAddr = shmat( s_ShmId, NULL, 0 );
	if( NULL == pShmAddr )
	{
		printf( "main:shmat() failed!!\n" );
		return FAILURE;
	}

	if( bIsMaster ) //�����ڴ���ʾ��ʼ��
	{
		memset( pShmAddr, 0x00, iShmSize );
	}

	// Ϊ�������ݷ��乲���ڴ���ʼ��ַ
	for( i = 0; i < SHM_ARRAY_SIZE; i++ )
	{
		s_atShareMem[i].pShmAddr = pShmAddr;
		printf( "shm adress:0x%x\n", (unsigned int )s_atShareMem[i].pShmAddr );
		pShmAddr = (void*)( (int)pShmAddr + s_atShareMem[i].iSize );
	}
	printf( "iShmId=%d \n", s_ShmId );
	return s_ShmId;
}

/***********************************************************
* Description:    ��ȡ�����ڴ��ַ
***********************************************************/
static void * get_shm_addr( int dtype )
{
	if( 0 <= dtype && dtype < SHM_TYPE_NUM )
	{
		return
		    s_atShareMem[dtype].pShmAddr;
	}

	return NULL;
}

/***********************************************************
* Description: �����ź���   
bIsMaster Ϊ�洴���������ȡ
***********************************************************/
static int create_semaphore( BOOLEAN bIsMaster )
{
	int		i;
	int		iSemKey;
	char	acFile[256] = { 0 };

	// ���ļ����ڻ�ȡ�����ڴ��key
	sprintf( acFile, "%s%s", APP_DIR, SEM_CONF );
	check_file_exist( acFile );
	iSemKey = ftok( acFile, SEM_KEY_ID );

	if( iSemKey == (key_t)-1 )
	{
		printf( "create_semaphore: ftok() for sem failed!!\n" );
		return FAILURE;
	}

	if( bIsMaster )
	{
		// ���������û��ɶ�д���ź���
		s_SemId = semget( iSemKey, SEM_NUM, 0666 | IPC_CREAT );
		if( -1 == s_SemId )
		{
			printf( "get_share_memory:shmget() failed!!\n" );
			return FAILURE;
		}
		// ��ʼ���ź�����
		//ʹ��forѭ����SETVAL��������ÿ���ź���ֵΪ1�����ɻ��
		for( i = 0; i < SEM_NUM; i++ )
		{
			if( semctl( s_SemId, i, SETVAL, 1 ) < 0 )
			{
				return FAILURE;
			}
		}
	}else 
	{
		s_SemId = semget( iSemKey, SEM_NUM, 0666 );
	}
	printf( "s_SemId=%d \n", s_SemId );
	return s_SemId;
}

/***********************************************************
* Description:    ɾ�������ڴ�
***********************************************************/
int  del_shm( )
{
	int rc;
	rc = shmctl( s_ShmId, IPC_RMID, NULL );
	if( FAILURE == rc )
	{
		printf( "del_shm: shmctl() failed!!\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:   �Ե����ź���V�������ͷ���Դ;
***********************************************************/
static int unlock_sem( int slNo )
{
	struct sembuf tSem;
	tSem.sem_num	= slNo;
	tSem.sem_op		= 1;
	tSem.sem_flg	= SEM_UNDO;
	return
	    semop( s_SemId, &tSem, 1 );
}

/***********************************************************
* Description:   
   �Ե����ź���P������ȡ����Դ
   ʹ��SEM_UNDOѡ���ֹ�ý����쳣�˳�ʱ���ܵ�������
***********************************************************/
static int lock_sem( int slNo )
{
	struct sembuf tSem;
	tSem.sem_num	= slNo;
	tSem.sem_op		= -1;
	tSem.sem_flg	= SEM_UNDO;
	return
	    semop( s_SemId, &tSem, 1 );
}

/***********************************************************
* Description:   ��ȡ�ź�����ֵ
***********************************************************/
static int get_sem( int slNo )
{
	return
	    semctl( s_SemId, 0, GETVAL );
}

/***********************************************************
* Description:   ɾ���ź���
***********************************************************/
int del_sem( void )
{
	int rc;
	rc = semctl( s_SemId, 0, IPC_RMID );
	if( rc == -1 )
	{
		//printf("del_sem:semctl() remove id failed!!\n");
		perror( "del_sem:semctl() remove id failed!!\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:  	
	SNMP����ʹ�õ����ݸ��½ӿڣ�
    �������ݣ����ظ��µ��ֽ�����
    ʵ���ϣ���Щ��ʼ���Ĺ��������Բ��ô�����ʵ�֣�
    ��������ʵ����Ŀ�У����������ᶨ��������Ҫ�ļ���
    ����������޷Ǿ��Ǹ�������������ԣ�
    ���������ͣ��ֽڳ��ȣ���дȨ�ޣ�����������ȱʡֵ�ȵȡ�
    ��ݶ����ļ����ǿ��Գ�֮Ϊϵͳ��ض���������ֵ䡣
    ����ֻҪ�����������ֵ�����һ��������нṹ��һ�µĶ����Ƶ��ļ���
    ÿ�γ�ʼ��ʱֻҪֱ�Ӷ�ȡ���ļ������ݵ������ڴ�Σ��������ϵͳ�ĳ�ʼ���Ĺ����� 
***********************************************************/
int  updata_cellvalue( int dType, int no, int ll, void* pV, int diretion )
{
	T_ShmCellVal	shmVal;
	int				ret = 0;
	if( no < 0
	    || ll < 0
	    || NULL == pV
	    || ( dType < SHM_PARADATA || dType >= SHM_TYPE_NUM )
	    )
	{
		printf( "init_shm_cellvalue failed!!\n" );
		return 0;
	}

	bzero( (void*)&shmVal, (size_t)sizeof( T_ShmCellVal ) );
	shmVal.iNo	= no;
	shmVal.iLen = ll;

	// ���û��ȡ
	// ����ʱ: pV -> shmVal.uValue ,��ȡʱ�������
	if( TO_SHM == diretion )
	{
		app_memcpy( pV, &shmVal.uValue, ll, diretion );
	}

	ret = update_shm_data( dType, &shmVal, diretion );

	// ��ȡʱ: shmVal.uValue -> pv,����ʱ�������
	if( (FROM_SHM == diretion) && (0 < ret) )
	{
		ret = app_memcpy( pV, &shmVal.uValue, ret, diretion );
	}

	return ret;
}


/***********************************************************
* Description:  
	SNMP����ʹ�õ����ݸ��½ӿڣ�
	pValue �������T_ShmCellVal ָ��,��Ϊ���������
	���ؿ����ĳ���;
***********************************************************/
 int update_shm_data( int dType, T_ShmCellVal *pValue, int diretion )
{
	int			iLen		= 0;
	char		      *pShmAddr	= (char*)get_shm_addr( dType );
	T_MapTable   *pMapTable= get_maptable( dType );
	T_ShmCellVal * pV		= pValue;

	if( NULL == pShmAddr || NULL == pV || NULL == pMapTable)
		return 0;
	if( pV->iNo >= get_maxobj_num(dType) )
		return 0;

	// ȡָ��λ�õĵ�Ԫ��ַ
	pShmAddr += pMapTable[pV->iNo].iOffset;
	iLen	= MIN( pMapTable[pV->iNo].iLen, pV->iLen );
	
	lock_sem( dType );
		app_memcpy( &pV->uValue, pShmAddr, iLen, diretion );
	unlock_sem( dType );

	return iLen;
}


/*
   �����������ݣ�
   ����Ӳ������ֻ�м������ݵĲ����ṹ��
   ʵ���У�һ��ϵͳ�Ĳ���ԶԶ��ֻ��Щ�������ʹ�����ַ�ʽ���϶�����һ���õĽ��������
   һ������£����ڶ�����ݵĴ���ʽ����ı�����ʽ��ʹ��ѭ����
   ��ʹ��ѭ����ȻҪ������е��������͵Ĵ���ʽ��һ����
   ��������˼·������ı��뷽ʽ�Ƕ���ͨ�õĽṹ������Ӷ���Ŀɹ�ѭ��ͳһ�������Ϣ����Ψһ��ʶ�ķ�����
   �������ݵĴ����Ϊ����ı��뼼���ǽ������Ϊ����ֵ�����ԡ���ʶ��
   �������ֲַ��˼�룬���е����ݶ�����Ϊͳһ�Ĵ������ˣ�
   ����Ͳ������ˣ����ϻ����ƺ������ˣ�
 */

/***********************************************************
* Description:   ����ͨ�ýṹ���빲���ڴ������
***********************************************************/
static void _update_data(void* pData, int dType,int direction)
{
	char *pAddr = NULL;
	int 	 i = 0;
	T_MapTable *pMap = get_maptable( dType );
	if( NULL == pMap || NULL == pData ) return ;
	
	for(i = 0; i < s_acMaxObjNum[dType]; i++ )
	{
	    pAddr = (char *)pData;
		updata_cellvalue( dType,i,pMap[i].iLen, 
			                 pAddr+pMap[i].iOffset, direction);
	}

	return;
}

/***********************************************************
* Description:   �����ṹ������빲���ڴ����ݸ���
***********************************************************/
static void update_para_data(T_ParaData* pData, int direction)
{
	return 
		_update_data(pData,SHM_PARADATA,direction);

}

/***********************************************************
* Description:   ʵʱ���ݽṹ������빲���ڴ����ݸ���
***********************************************************/
static void update_realtime_data(T_RealData* pData, int direction)
{
	
	void * pShmVal = get_shm_addr( SHM_REALDATA );
	if( NULL == pShmVal || NULL == pData ) return ;

	lock_sem( SHM_REALDATA );
		app_memcpy( pData, pShmVal, sizeof(T_RealData),direction);
	unlock_sem( SHM_REALDATA );
	
	return ;
}

/***********************************************************
* Description:  �澯���ݽṹ������빲���ڴ����ݸ���
***********************************************************/
static void update_alarm_data(T_AlarmData* pData, int direction)
{
	return 
		_update_data(pData,SHM_ALARM,direction);

}


/***********************************************************
* Description:   ҵ�����ʹ��: �ṹ������빲���ڴ����ݸ��£�
pStrVal,Ϊҵ������нṹ�������
dir,ȡֵΪFROM_SHM��TO_SHM,�ֱ��ʾ��д�����ڴ棻
***********************************************************/
int update_data(void* pStrVal, int dType, int dir)
{
	if( NULL == pStrVal ) return FAILURE;
	
	if( SHM_PARADATA == dType)
		update_para_data( pStrVal, dir );
	else if( SHM_REALDATA == dType )
		update_realtime_data(pStrVal, dir);
	else if( SHM_ALARM== dType )
		update_alarm_data(pStrVal, dir);
	else
		return FAILURE;

	return SUCCESS;
}

/***********************************************************
* Description:   ҵ�����ʹ��: 
�ӹ����ڴ��ж����ݸ��µ��ṹ��
pStrVal,Ϊҵ������нṹ�����ָ��;
dType:��������
***********************************************************/
int app_get_data(void* pStrVal, int dType)
{	
	return 
		update_data(pStrVal, dType,FROM_SHM);

}

/***********************************************************
* Description:   ҵ�����ʹ��: 
�ӽṹ��д�������ڴ���
pStrVal,Ϊҵ������нṹ�����ָ��;
dType:��������
***********************************************************/
int app_set_data(void* pStrVal, int dType)
{	
	return 
		update_data(pStrVal, dType,TO_SHM);

}


/***********************************************************
* Description:   snmp����ʹ��: 
�ӹ����ڴ��ж����ݵ�pV
dType:��������
no:���
ll:�ֽڳ���
pV:������ַ
***********************************************************/
int snmp_get_data( int dType, int no, int ll, void* pV )
{
	return
		updata_cellvalue(dType,no,ll,pV,FROM_SHM);

}

/***********************************************************
* Description:   snmp����ʹ��: 
��pV����д�뵽�����ڴ���
dType:��������
no:���
ll:�ֽڳ���
pV:������ַ
***********************************************************/
int snmp_set_data( int dType, int no, int ll, void* pV )
{
	return
		updata_cellvalue(dType,no,ll,pV,TO_SHM);

}


/***********************************************************
* Description:�����ڴ���ź�����ʼ��
isMasterָʾ�Ƿ�Ϊ��ҵ����̣��ý��̸��𴴽��ͳ�ʼ��
***********************************************************/
void init_shm_sem(BOOLEAN isMaster)
{
	if( FAILURE == init_share_memory( isMaster ) )
	{
		exit (-1);
	}
	if( FAILURE == create_semaphore( isMaster ) )
	{
		exit (-1);
	}
}


