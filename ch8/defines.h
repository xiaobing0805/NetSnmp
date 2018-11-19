/************************************************************
 * Copyright (C)	GPL
 * FileName:		defines.h
 * Author:		�Ŵ�ǿ
 * Date:			2014-07
 ***********************************************************/
#ifndef DEFINES_H
#define DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */


/* ����ʹ����typedef�����Ľṹ���ڴ�ռ�,
   ���㣬���ؽṹ��ָ��*/
#define MALLOC_TYPEDEF_STRUCT( td ) (td*)calloc( 1, sizeof( td ) )


/* ����δʹ����typedef�����Ľṹ���ڴ�ռ�,
   ���㣬���ؽṹ��ָ��*/
#define MALLOC_STRUCT( s ) (struct s *)calloc( 1, sizeof( struct s ) )

// �ͷŷǿյ�ָ����ڴ�ռ䣬ͬʱ��NULL
#define NMSAPP_FREE( s )	do { if( s ){ free( (void*)s ); s = NULL; } } while( 0 )
#define MIN( a, b )			( ( a ) > ( b ) ? ( b ) : ( a ) )

#define TRUE	1
#define FALSE	0
#define SUCCESS 0
#define FAILURE -1

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  DEFINES_H  */
