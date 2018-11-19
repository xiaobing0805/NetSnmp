/************************************************************
 * Copyright (C)	GPL
 * FileName:		 list.h:�ṩ��������ĩβ��Ӻ�����ѭ�������3���ӿ�
 * Author:		�Ŵ�ǿ
 * Date:			2014-07
 ***********************************************************/
#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif                          /*__cplusplus */

typedef struct  _list_node
{
	void* data;                
	struct _list_node *next;
} T_ListNode;

struct  _list
{
	T_ListNode	*head;        // ָ�������һ���ڵ�
	T_ListNode	*tail;          // ָ���������һ���ڵ�
	int			size;          // �����еĽڵ���	
};
typedef struct _list T_List;

//list_foreach  �����һ���������뵽context; dataΪlist�е�data
typedef int (*LIST_FUN_CUSTOM)( void* data, void* context );

#define PRINT_STRING	( 0 )
#define PRINT_INT		( 1 )
#define list_size( pList ) ( ( pList )->size )

T_List * create_list( void );


void free_list( T_List * pList );


int list_append( T_List *pList, void* data, LIST_FUN_CUSTOM fun_custom );


int list_foreach( T_List *pList, LIST_FUN_CUSTOM fun, void* ctx );


// which = PRINT_INT,PRINT_STRING
void list_printf( T_List *pList, int which );


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  LIST_H  */
