#ifndef MEMLINK_SYNCLOG_H
#define MEMLINK_SYNCLOG_H

#include <stdio.h>
#include <limits.h>


#define SYNCLOG_OK   0
#define SYNCLOG_FULL 1

#define SYNCLOG_NAME "bin.log"


/**
 * header and index area are mapped in memory address space.
 */
typedef struct _synclog
{
    char    filename[PATH_MAX]; // file path
    int     fd;                 // open file descriptor
    char    *index;             // mmap addr
    int     len;                // mmap len
    //int     headlen;            // header length
    unsigned int    index_pos;  // last index pos
    unsigned int    pos;        // last write data pos
}SyncLog;

SyncLog*    synclog_create();
SyncLog*    synclog_open(char *filename);
int         synclog_new(SyncLog *slog);
int         synclog_validate(SyncLog *slog);
int         synclog_write(SyncLog *slog, char *data, int datalen);
void        synclog_destroy(SyncLog *slog);
int         synclog_rotate(SyncLog *slog);
int			synclog_version(SyncLog *slog, unsigned int *logver);
int         synclog_lastlog();
int			synclog_prevlog(int curid);

#endif