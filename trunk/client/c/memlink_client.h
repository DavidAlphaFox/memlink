#ifndef MEMLINK_CLIENT_H
#define MEMLINK_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "common.h"

#define MEMLINK_READER  1
#define MEMLINK_WRITER  2
#define MEMLINK_ALL     3

#define MAX_PACKAGE_LEN 1024000
//#define MEMLINK_TAG_DEL     1
//#define MEMLINK_TAG_RESTORE 0

typedef struct _memlink_client
{
    char    host[16];
    int     read_port;
    int     write_port;
    int     readfd;
    int     writefd;
    int     timeout;
}MemLink;

typedef struct _memlink_count
{
	unsigned int	visible_count;
	unsigned int	tagdel_count;
}MemLinkCount;

typedef struct _memlink_item
{
    struct _memlink_item    *next;
    char                    value[256];
    char                    mask[256];
}MemLinkItem;

typedef struct _memlink_result
{
    int             count;
    int             valuesize;
    int             masksize;
    MemLinkItem     *root;
}MemLinkResult;

int         memlink_result_parse(char *retdata, MemLinkResult *result);
void        memlink_result_free(MemLinkResult *result);

MemLink*    memlink_create(char *host, int readport, int writeport, int timeout);
void        memlink_destroy(MemLink *m);
void		memlink_close(MemLink *m);

int         memlink_cmd_ping(MemLink *m);
int         memlink_cmd_dump(MemLink *m);
int			memlink_cmd_clean(MemLink *m, char *key);
int			memlink_cmd_stat(MemLink *m, char *key, MemLinkStat *stat);
int			memlink_cmd_stat_sys(MemLink *m, MemLinkStatSys *stat);
int			memlink_cmd_create(MemLink *m, char *key, int valuelen, char *maskstr, 
                                unsigned char listtype, unsigned char valuetype);

int			memlink_cmd_create_list(MemLink *m, char *key, int valuelen, char *maskstr);
int			memlink_cmd_create_queue(MemLink *m, char *key, int valuelen, char *maskstr);
int			memlink_cmd_create_sortlist(MemLink *m, char *key, int valuelen, char *maskstr, 
                                        unsigned char valuetype);
int			memlink_cmd_del(MemLink *m, char *key, char *value, int valuelen);
int			memlink_cmd_insert(MemLink *m, char *key, char *value, int valuelen, 
                               char *maskstr, int pos);

int			memlink_cmd_lpush(MemLink *m, char *key, char *value, int valuelen, char *maskstr);
int			memlink_cmd_rpush(MemLink *m, char *key, char *value, int valuelen, char *maskstr);
int			memlink_cmd_lpop(MemLink *m, char *key, int num, MemLinkResult *result);
int			memlink_cmd_rpop(MemLink *m, char *key, int num, MemLinkResult *result);

int			memlink_cmd_move(MemLink *m, char *key, char *value, int valuelen, int pos);
int			memlink_cmd_mask(MemLink *m, char *key, char *value, int valuelen, char *maskstr);
int			memlink_cmd_tag(MemLink *m, char *key, char *value, int valuelen, int tag);
int			memlink_cmd_range(MemLink *m, char *key, int kind, char *maskstr, 
                              int frompos, int len, MemLinkResult *result);
int         memlink_cmd_rmkey(MemLink *m, char *key);
int         memlink_cmd_count(MemLink *m, char *key, char *maskstr, MemLinkCount *count);
int         memlink_cmd_insert_mvalue(MemLink *m, char *key, MemLinkInsertVal *values, int num);

//add by lanwenhong
int         memlink_cmd_del_by_mask(MemLink *m, char *key, char *maskstr);

int			memlink_cmd_sortlist_insert(MemLink *m, char *key, char *value, int valuelen, char *maskstr);
int			memlink_cmd_sortlist_range(MemLink *m, char *key, int kind, char *maskstr, 
                              char *valmin, unsigned char vminlen, 
                              char *valmax, unsigned char vmaxlen, MemLinkResult *result);

int			memlink_cmd_sortlist_del(MemLink *m, char *key, char *valmin, unsigned char vminlen, 
                            char *valmax, unsigned char vmaxlen);
int         memlink_cmd_sortlist_count(MemLink *m, char *key, char *maskstr, 
                            char *valmin, unsigned char vminlen, 
                            char *valmax, unsigned char vmaxlen, MemLinkCount *count);


int         memlink_cmd_insert_mkv(MemLink *m, MemLinkInsertMkv *mkv, int *kindex, int *vindex);
int         memlink_mkv_destroy(MemLinkInsertMkv *mkv);
int         memlink_mkv_add_key(MemLinkInsertMkv *mkv, MemLinkInsertKey *keyobj);
int         memlink_ikey_add_value(MemLinkInsertKey *keyobj, MemLinkInsertVal *valobj);
MemLinkInsertMkv*         memlink_imkv_create(); 
MemLinkInsertKey*         memlink_ikey_create(char *key, unsigned int keylen);
MemLinkInsertVal*         memlink_ival_create(char *value, unsigned int valuelen, char *maskstr, int pos);

#endif


