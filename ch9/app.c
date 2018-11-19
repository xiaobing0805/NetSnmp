/************************************************************
 * Copyright (C)	GPL
 * FileName:		app.c
 * Author:		�Ŵ�ǿ
 * Date:			2014-08
 ***********************************************************/

//��ҵ�����
// gcc -g -Wall app.c snmpipc.c -o app
// gcc -g -Wall app.c -lsnmpipc -o app
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include "snmpipc.h"

static T_RealData	s_tRealData;
static T_ParaData	s_tParaData = {
	.a	= 99,
	.c	= { 88,	77,	 66 },
	.b	= "192.168.43.132",
};
static T_AlarmData	s_tAlarmData ={
	.alarm1 = 1,
	.alarm2 = "This is An alarm",
	.alarmCounter = 0,
};


int	REAL_DATA_SIZE = sizeof( T_RealData ) / sizeof( int );
int ALARM_DATA_SIZE = sizeof( T_AlarmData ) / sizeof( int );


/***********************************************************
* Description:    ��ӡʵʱ����
***********************************************************/
void print_int_data(int *pIntV,int nn)

{
	int i;
	int *pInt = (int*)pIntV;
	if( NULL == pInt || nn <= 0) return ;

	printf( "--int data: " );
	for( i = 0; i < nn; i++ )
	{
		printf( " %d, ", *pInt );
		pInt++;
	}
	printf( "\n\n" );
}

/***********************************************************
* Description:    ����ʵʱ����
***********************************************************/
void set_int_data(int *pIntV,int nn )
{
	int i;
	int *pInt = (int*)pIntV;
	if( NULL == pInt || nn <= 0) return ;

	for( i = 0; i < nn; i++ )
	{
        if( nn == ALARM_DATA_SIZE)// only example
		( *pInt ) += i; // ģ��澯����
		else
		( *pInt ) += 2*i;// ģ��ʵʱ����
		
		pInt++;
	}
	printf( "----------------------\n\n" );
}


/***********************************************************
* Description:    ��ӡ����
***********************************************************/
void print_para_data( void )
{
	printf( "--para data is :\n" );
	printf( "%d,", s_tParaData.a );
	printf( "%d,%d,%d,", s_tParaData.c[0], s_tParaData.c[1], s_tParaData.c[2] );
	printf( "%s\n\n", s_tParaData.b );
	//printf( "%d,\n", s_tParaData.g );
}

/***********************************************************
* Description:    ��ӡ����
***********************************************************/
void print_alarm_data( void )
{
	printf( "--alarm data is :\n" );
	printf( "%d,", s_tAlarmData.alarm1);
	printf( "%s,", s_tAlarmData.alarm2);
	printf( "%d\n\n", s_tAlarmData.alarmCounter);
	//printf( "%d,\n", s_tParaData.g );
}

/***********************************************************
* Description:    
 ��ҵ�����
 app sleep loop
 ./app 10 5
***********************************************************/
int main( int argc, char **argv )
{

	int	nn = 0;
	int sec =10; // ������
	int loop = 5;

	// ��ʼ�������ڴ�
	//init_shm_sem( MASTER );
	init_shm_sem_master();

	if( argc >= 2 )
	{
		sec = atoi( argv[1] );
	}
	if( argc >= 3 )
	{
		loop = atoi( argv[2] );
	}


	//-----ģ��ʵʱ���ݲ���------------------
	while( nn < loop )
	{
		/* ------- ģ��ʵʱ���ݸ��� ------- */
		if( nn%3 == 0)
		{
			printf( "----- realtime data -----\n" );
			app_set_data(&s_tRealData,SHM_REALDATA);
			print_int_data( (int*)&s_tRealData,REAL_DATA_SIZE);
			sleep( sec );                 // ��SNMP��ȡ������ʱ��
		}

		// �������ݵ������ڴ�
		if( nn%3 == 0 )
		{
			printf( "----- realtime data -----\n" );
			set_int_data((int*)&s_tRealData,REAL_DATA_SIZE);
			print_int_data( (int*)&s_tRealData,REAL_DATA_SIZE);
			app_set_data( &s_tRealData ,SHM_REALDATA );
			sleep( sec );                 // ��SNMP��ȡ������ʱ��
		}


		/* ---------ģ���������------------------ */
		if( nn )
		{
			printf( "----- parameter data -----\n" );
			printf( "before set parameter:\n" );
			//app_get_data(&s_tParaData,SHM_PARADATA);
			print_para_data( );
			
			app_set_data( &s_tParaData ,SHM_PARADATA );
			printf( "after set parameter:\n" );
			print_para_data( );
			sleep( 2*sec ); //�ȴ�SNMP�����ȡ����
		}

		if( nn )
		{
			printf( "before set parameter:\n" );
			app_get_data(&s_tParaData,SHM_PARADATA);
			print_para_data( );
			
			s_tParaData.c[0] = 12345;
			strcpy( s_tParaData.b, "1.1.1.1" );
			app_set_data(&s_tParaData,SHM_PARADATA );
			printf( "after set parameter:\n" );
			print_para_data( );
			sleep( 2*sec );
		}

		if( nn )
		{
			printf( "get parameter:\n" );
			app_get_data(&s_tParaData,SHM_PARAMETER);
			print_para_data( );
		}

		/* ----- �澯���� ------ */
	    if( nn  )
		{
			printf( "----- alarm data -----\n" );
			//print_int_data( (int*)&s_tAlarmData,ALARM_DATA_SIZE);
			s_tAlarmData.alarmCounter += 1;
			app_set_data( &s_tAlarmData ,SHM_ALARM );
			print_alarm_data();
			sleep( sec );                 // ��SNMP��ȡ������ʱ��
			app_get_data( &s_tAlarmData ,SHM_ALARM );
			print_alarm_data();
		}
		nn++;
	}
	
	// �澯����
	//init_alarm_shm();

	
	sleep( sec ); // �ȴ��������
	del_shm( );
	del_sem( );


	return 0;
}


