/************************************************************
 * Copyright (C)	GPL
 * FileName:		snmpipc.h
 * Author:		�Ŵ�ǿ
 * Date:			2014-08
 ***********************************************************/

#ifndef SNMPIPC_H
#define SNMPIPC_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */

// ��֤��Ŀ¼�Ѿ�����
#define APP_DIR		"/usr/local/etc/app/shm/"
#define SHM_CONF	"shm.conf"
#define SEM_CONF	"sem.conf"

#define SHM_KEY_ID	1
#define SEM_KEY_ID	1

#define SUCCESS 0
#define FAILURE -1
typedef int BOOLEAN;

// �������Ͷ���
typedef enum
{
	// �� 0 ��ʼ�������飬�ź���ƥ��
	SHM_PARADATA = 0,                   // ������������
	SHM_REALDATA,                       // ʵʱ��������
	SHM_ALARM,                          // �澯��������
	SHM_CTRL,                           // ������������
} SHM_TYPE;

typedef struct
{
	int iSize;                          //��ʵ�ṹ���С��ͨ�ýṹ���С
	void *pShmAddr;
} T_ShareMem;

#define SHM_TYPE_NUM ( SHM_CTRL + 1 )   // ��Ҫ�����ڹ����ڴ��е�������������

// �ź�������:ÿ����������һ���ź���
#define SEM_NUM SHM_TYPE_NUM

#define TO_SHM		( 0 )
#define FROM_SHM	( 1 )

#define SLAVE	( 0 )
#define MASTER	( 1 )

#define MAX_CHAR_LEN (32)       // ϵͳ���ַ���������󳤶ȣ�4�ı���
typedef   union
{
	int		iValue;                 // ͳһ����������
	char	acValue[MAX_CHAR_LEN];  // ͳһ���ַ�����������
} U_Value;


typedef struct
{
	int iNo;
	int iOffset;
	int iLen;
} T_MapTable;

/*��������: ģ�����Σ��ַ��ͣ��򵥱�һ���е��� */
#define C_ROW_NUM (3)
typedef struct
{
	int		a;                  // 0
	char	b[MAX_CHAR_LEN];    // 1
	int		c[C_ROW_NUM];       // 2,3,4
	// ...
} T_ParaData;
typedef enum
{
	// �� 0 ��ʼ,����
	PARA_A = 0,
	PARA_B ,
	PARA_C1 ,//2,c�ڽṹ���е���ʼλ�����
	PARA_C2 ,
	PARA_C3,
}E_PARA;

#define  C_OFFSET (PARA_C1)

/* ʵʱ����,ȫ����ʾΪ���Σ�ʵ����������侫�Ƚ��д���ֻ��
   SNMP��NMS����ʾ�ɶ���OID�﷨����
   �����ע�����ֱ�ʾ�ñ����ڸ��������Ͳ��ֵĹ����ڴ��е�λ�á���Ψһ����ʵʱ���ݹ���λ�õ���ʼ��ַΪ
   0x1234����ô0x1234+0�ͱ�ʾ����a�ĵ�ַ
 */


/*ʵʱ����:ģ�����Σ��ַ��ͣ��򵥱���ж��� */
#define GROUP_NUM (2)// ʾ��ĳ����-�����Ԫ��
struct _aGroup
{
	int x;
	int y;
	// ...
};

/*
���No.	x y
��һ��	0 1
�ڶ���	2 3
z:		4
*/
typedef struct
{
	struct _aGroup xy[GROUP_NUM];
	int z;                          // 4
	// ...
} T_RealData;
typedef enum
{
	// �� 0 ��ʼ,����
	XY0X = 0,
	XY0Y ,
	XY1X ,//2
	XY1Y ,
	REALZ,
}E_REALTIMEDATA;


typedef struct
{
	int alarm1;                       // 0
	//int alarm2;                       // 1
	char alarm2[MAX_CHAR_LEN];
	int  alarmCounter;                 // 2
	// ...
} T_AlarmData;
typedef enum
{
	// �� 0 ��ʼ,����
	ALARM1 = 0,
	ALARM2,
	ALARM_COUNTER,//2
}E_ALARM;


// Ϊ�˽�ʡ�ڴ棬�ýṹ�岢û��Ӧ�õ�ʵʱ������
typedef struct
{
	// ��������Ҫ�ֶ�ά������Ȼ���ǿ��Խ��ɳ����һЩǰ�˽ű��Զ�����
	int		iNo;    // �����ı��ֵ(�������ڲ������е�����������0��ʼ)
	int		iLen;   // ���ݵ��ֽڳ��ȣ����������Ϊsizeof(int)���ñ����б�Ҫ�𣡣�
	U_Value uValue; //������ֵ
} T_ShmCellVal;

#ifndef  MIN
#define  MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

// ������
int del_sem( void );
int del_shm( void );


int app_get_data(void* pStrVal, int dType);
int app_set_data(void* pStrVal, int dType);
int snmp_get_data( int dType, int no, int ll, void* pV );
int snmp_set_data( int dType, int no, int ll, void* pV );

void init_shm_sem(BOOLEAN isMaster);

	
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  SNMPIPC_H  */

