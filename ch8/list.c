/************************************************************
 * Copyright (C)	GPL
 * FileName:		list.c
 * Author:		 �Ŵ�ǿ
 * Date:			2014-07
 ***********************************************************/
// gcc -Wall list.c -o list
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "list.h"
#include "defines.h"


/***********************************************************
* Description:    ��������ڵ�
***********************************************************/
T_ListNode *create_list_node( void * data )
{
	T_ListNode *newNode = MALLOC_TYPEDEF_STRUCT( T_ListNode );

	if( NULL != newNode )
	{
		newNode->data	= data;
		newNode->next	= NULL;
	}
	return newNode;
}

/***********************************************************
* Description:   �ͷ�����ڵ�
***********************************************************/
void free_list_node( T_ListNode *pNode )
{
	if( NULL != pNode )
	{
		pNode->next = NULL;
		NMSAPP_FREE( pNode );
		printf( "--free node is ok\n" );
	}
	return;
}

/***********************************************************
* Description:   ��������
***********************************************************/
T_List * create_list( void )
{
	T_List *list = MALLOC_TYPEDEF_STRUCT( T_List );

	if( NULL != list )
	{
		list->head	= NULL;
		list->tail	= NULL;
		list->size	= 0;
	}

	return list;
}

/***********************************************************
* Description:   �ͷ�����
***********************************************************/
void  free_list( T_List *pList )
{
	T_ListNode	*it		= pList->head;
	T_ListNode	*next	= NULL;
	while( NULL != it )
	{
		next = it->next;
		free_list_node( it );
		it = next;
	}
	pList->head = NULL;
	pList->tail = NULL;
	pList->size = 0;
	free( pList );
	printf( "--free_list ed \n" );
}

/***********************************************************
* Description:   ��������ӵ�����pListβ��
* void* data �Ѿ����ⲿ������˿ռ�����ݣ�
*	׼���浽����������
* LIST_FUN_CUSTOM fun_custom�û��Զ��庯����
*	 �����Ƿ񽫸�������ӵ�������
* ����ֵ��FAILURE-���������Ѿ������void* data�����ͷţ�
*            SUCCESS-��ʾ����ڵ���ӳɹ�
***********************************************************/
int list_append( T_List *pList, void* data, LIST_FUN_CUSTOM fun_custom )
{
	T_ListNode* newNode = NULL;

	if( NULL == pList )
	{
		return FAILURE;
	}
	if( NULL == ( newNode = create_list_node( data ) ) )
	{
		return FAILURE;
	}

	if( NULL != pList->head )           // �ǿ�����
	{
		if( ( NULL != fun_custom )
		    && ( FAILURE == fun_custom( pList, data ) )
		    )
		{
			printf( "list_append stoped by fun_custom\n" );
			free_list_node( newNode );  // ����ֻ�ͷ�������Ľڵ�
			return FAILURE;
		}else //��������ӽڵ�
		{
			pList->tail->next	= newNode;
			pList->tail			= newNode;
		}
	}else // ������
	{
		pList->tail = newNode;
		pList->head = newNode;
		printf( "======= add head ===========\n" );
	}

	pList->size++;
	//printf( "add a node to list[%d]~~ \n", pList->size );
	return SUCCESS;
}

/***********************************************************
* Description:    ��list�е�ÿ�� data ��ʹ�� fun �������д���
* ����ctx�Ƕ��⴫�������
***********************************************************/
int list_foreach( T_List *list, LIST_FUN_CUSTOM fun, void* ctx )
{
	int			ret = SUCCESS;
	T_ListNode	*it = list->head;
	while( NULL != it )
	{
		ret = fun( it->data, ctx );
		if( FAILURE == ret )
		{
			printf( "--list_foreach i have returned FAILURE\n" );
			return ret;
		}
		it = it->next;
	}
	return ret;
}

/***********************************************************
* Description:   ��ӡ�ַ���
***********************************************************/
static int  print_string( void* data, void* ctx )
{
	printf( "%s \n", (char*)data );
	return SUCCESS;
}

/***********************************************************
* Description:    ��ӡ����
***********************************************************/
static int  print_int( void* data, void* ctx )
{
	printf( "%d ", (int)data );
	return SUCCESS;
}

/***********************************************************
* Description:   ��ӡ����
***********************************************************/
void list_printf( T_List *pList, int which )
{
	printf( "in list_printf======\n" );
	switch( which )
	{
		case PRINT_INT:
			list_foreach( pList, print_int, NULL );
			break;
		case PRINT_STRING:
			list_foreach( pList, print_string, NULL );
			break;
		// else to add here...
		default:
			printf( "no data to printf in list\n" );
	}
	return;
}

/***********************************************************
* Description:    ���
***********************************************************/
static int sum_cb( void* ctx, void* data )
{
	long long* result = ctx;
	*result += (int)data;

	return SUCCESS;
}

/***********************************************************
* Description:   �Ƚ��ַ���
***********************************************************/
static int compare_string_in_list( void* ctx, void* data )
{
	if( 0 == strcmp( (char*)ctx, (char*)data ) )
	{
		printf( "--same: %s\n", (char*)data );
		return FAILURE;
	}else
	{
		return SUCCESS;
	}
}

/***********************************************************
* Description:    �Ƚ��ַ���
***********************************************************/
static int cmp_cb_string_list( void* data, void* ctx )
{
	return list_foreach( ctx, compare_string_in_list, data );
}

/***********************************************************
* Description:    �Ƚ�����
***********************************************************/
int compare_int_in_list( void* data, void* ctx )
{
	if( (int)data == (int)ctx )
	{
		return FAILURE;
	} else
	{
		return SUCCESS;
	}
}

/***********************************************************
* Description:    �Ƚ�����
***********************************************************/
static int cmp_cb_int( void* ctx, void* data )
{
	return list_foreach( ctx, compare_int_in_list, data );
}

