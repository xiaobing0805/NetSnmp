/************************************************************
 * Copyright (C)	GPL
 * FileName:		tools.c
 * Author:		�Ŵ�ǿ
 * Date:			2014-07
 ***********************************************************/
// gcc -Wall tools.c -o tools

#include "tools.h"
#include "defines.h"


/***********************************************************
* Description:    ���ļ�
***********************************************************/
int lock_file( int iLkFd )
{
	if( iLkFd < 0 )
	{
		return FAILURE;
	}

	//�����ļ�������close-on-exec��־
	fcntl( iLkFd, F_SETFD, FD_CLOEXEC );

	// ������LOCK_EX
	if( 0 > flock( iLkFd, LOCK_EX | LOCK_NB ) )
	{
		printf( "lock file failed !\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:    �����ļ�
***********************************************************/
int unlock_file( int iLkFd )
{
	if( iLkFd < 0 )
	{
		return FAILURE;
	}

	// ������LOCK_UN
	if( 0 > flock( iLkFd, LOCK_UN | LOCK_NB ) )
	{
		printf( "unlock file failed !\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:   �ַ��� ɾ�����˵Ŀո�
***********************************************************/
char *trim_ends_space( char *pSrc )
{
	char	*pStr		= pSrc;
	char	*pStartPos	= NULL;                         // ����ǿո����ʼλ��
	int		len = 0;

	if( NULL != pSrc )
	{
		while( *pStr != 0 && isspace( (unsigned char)*pStr ) ) 
		{
			pStr++;                                     // ����ǰ���ո�
		}
		pStartPos	= pStr;                             // ����ǿո����ʼλ��
		len = strlen( pSrc ) - 1;
		pStr		= pSrc + len;    	// pStr ָ�򴮽�����ǰ��һ���ַ�
		while( isspace( *pStr ) && (len-- > 0 ))
		{
			pStr--;                                     // ������׺�ո�
		}
		*( pStr + 1 ) = '\0';                           // �ڿո�λ��������
	}
	return pStartPos;
}


 /***********************************************************
* Description:   �����ָ����������ߵ��ַ���:
* ����ַ�����return ����;
* �ұ��ַ�����saveptr����
***********************************************************/
static char * strtok_r_Ex( char *str, const char *delim, char **saveptr )
{
	char	*pLeft	= NULL;
	char	*pRight = NULL;
	char	*ptemp	= NULL;

	if( NULL == str
	    || NULL == delim
	    )
	{
		return NULL;
	}

	// ����ָ����ڵ�һ���ַ����������
	ptemp = trim_ends_space( str );
	if( 1 == strspn( ptemp, delim ) ) // delim == ptemp[0]
	{
		*saveptr = trim_ends_space( ptemp + 1 );
		printf( "delim in the first positon:%s\n\n", *saveptr );
		return NULL;
	}

	pLeft = strtok_r( str, delim, &pRight );
	if( NULL != pLeft )
	{
		pLeft = trim_ends_space( pLeft );
	}
	if( NULL != pRight )
	{
		pRight		= trim_ends_space( pRight );
		*saveptr	= pRight;
	}

	return pLeft;
}


 /***********************************************************
* Description:   delim:Ϊ�ָ����ַ����ķ���
* �����pLeftΪ�ָ�����ߵ��ַ�����
* pRightΪ�ұߵ��ַ�������Щ�ַ������Ѿ�ȥ����β�ո�
***********************************************************/
int parser_delim( const char *pIn, char *pDelim, char *pLeft, char *pRight )
{
	char	*pStr = NULL;
	char	*pToken		= NULL;
	char	*pSubToken	= NULL;
	int		ii			= 0;

	if( NULL == pIn
	    || NULL == pDelim
	    || NULL == pLeft
	    || NULL == pRight
	    )
	{
		return FAILURE;
	}

	ii = strlen( pIn ) + 1;

	// ���ƴ�������ַ���
	if( NULL == ( pStr = (char*)malloc( ii ) ) )
	{
		return FAILURE;
	}

	bzero( pStr, ii );
	memcpy( pStr, pIn, ii );

	// ����pDelim �ַ�ʱ�Ž��зָ�
	if( NULL != strstr( pStr, pDelim ) )
	{
		pToken = strtok_r_Ex( pStr, pDelim, &pSubToken );
		if( NULL != pToken )
		{
			strcpy( pLeft, pToken );
		}

		//printf("pSubToken:%s,",pSubToken);
		if( NULL != pSubToken )
		{
			strcpy( pRight, pSubToken );
		}
		//printf("pRight:%s\n\n",pRight);
	}else
	{
		printf( "parser_delim: delimiter '%s' not found !!\n", pDelim );
		free( pStr );
		return FAILURE;
	}

	free( pStr );
	return SUCCESS;
}

/***********************************************************
* Description:    �����ַ����е�IP��
* ��192.168.43.146:161 �е�192.168.43.146
***********************************************************/
char*  get_token_ip( const char* pval, char *ip ) // 
{
	char pRight[128] = { 0 };

	if( NULL != pval && NULL != ip )
	{
		if( NULL != strstr( pval, COLON ) ) //����
		{
			parser_delim( pval, COLON, ip, pRight );
		} else
		{
			strcpy( ip, pval );
		}
	}
	return ip;
}

