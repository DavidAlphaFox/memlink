#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <netdb.h>
#ifdef __linux
    #include <linux/if.h>
    #include <sys/un.h>
#endif
#include "memlink_client.h"
#include "logfile.h"
#include "zzmalloc.h"
#include "utils.h"

MemLink*    
memlink_create(char *host, int readport, int writeport, int timeout)
{
    MemLink *m;

    m = (MemLink*)zz_malloc(sizeof(MemLink));
    if (NULL == m) {
        DERROR("malloc MemLink error!\n"); 
        return NULL;
    }
    memset(m, 0, sizeof(MemLink));

    snprintf(m->host, 16, "%s", host);
    m->read_port  = readport;
    m->write_port = writeport;
    m->timeout    = timeout;

    return m;
}

static int memlink_wait(MemLink *m, int fdtype, int writing)
{
    if (m->timeout <= 0)
        return 1;

    int fd;

    if (fdtype == MEMLINK_READER) {
        fd = m->readfd;
    }else if (fdtype == MEMLINK_WRITER) {
        fd = m->writefd;
    }else{
        return -100;
    }

    if (fd <= 0) {
        return -200;
    }

    fd_set fds; 
    struct timeval tv;
    int n;

    tv.tv_sec  = (int)m->timeout;
    tv.tv_usec = (int)((m->timeout - tv.tv_sec) * 1e6);

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    if (writing)
        n = select(fd+1, NULL, &fds, NULL, &tv);
    else 
        n = select(fd+1, &fds, NULL, NULL, &tv);

    if (n < 0) 
        return -1;

    if (n == 0)
        return 1;

    return 0; 
}

static int
memlink_connect(MemLink *m, int fdtype)
{
    int ret;
    int sock;
    
    DINFO("memlink connect ...\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock) {
        DERROR("socket create error: %s\n", strerror(errno));
        return -1;
    }

    struct linger ling = {0, 0}; 
    ret = setsockopt(sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (ret != 0) {
        DERROR("setsockopt LINGER error: %s\n", strerror(errno));
    }   
    int flags = 1;
    ret = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    if (ret != 0) {
        DERROR("setsockopt NODELAY error: %s\n", strerror(errno));
    }

    struct sockaddr_in  sin; 

    sin.sin_family = AF_INET;
    if (fdtype == MEMLINK_READER) {
        sin.sin_port = htons((short)(m->read_port));
    }else if (fdtype == MEMLINK_WRITER) {
        sin.sin_port = htons((short)(m->write_port));
    }else{
        return -2;
    }

    if (m->host == NULL) {
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
    }else{
        sin.sin_addr.s_addr = inet_addr(m->host);
    }    

    do { 
        ret = connect(sock, (struct sockaddr*)&sin, sizeof(sin));
    } while (ret == -1 && errno == EINTR);  

    DINFO("connect ret: %d\n", ret);
    if (ret >= 0) {
        if (fdtype == MEMLINK_READER) {
            m->readfd = sock;
        }else{
            m->writefd = sock;
        }
    }else{
        close(sock);
    }

    return ret;
}

static int
memlink_read(MemLink *m, int fdtype, char *rbuf, int rlen)
{
    int rdlen = 0; 
    int nleft = rlen;
    int datalen = 0;
    int ret;
    int fd;
   
    if (fdtype == MEMLINK_READER) {
        fd = m->readfd;
    }else if (fdtype == MEMLINK_WRITER) {
        fd = m->writefd;
    }else{
        return -1;
    }
    DINFO("fd: %d\n", fd);
    
    if (fd <= 0) {
        DINFO("read fd connect ...\n");
        ret = memlink_connect(m, fdtype);
        if (ret < 0)
            return ret;
    }

    ret = readn(fd, rbuf, sizeof(short));
    DINFO("read head: %d\n", ret);
    if (ret != sizeof(short)) {
        DERROR("read head error!\n");
        return -2;
    }
    memcpy(&datalen, rbuf, sizeof(short));
    DINFO("read body: %d\n", datalen);

    ret = readn(fd, rbuf + sizeof(short), datalen);
    DINFO("readn return: %d\n", ret);
    if (ret != datalen) {
        DERROR("read data error!\n");
        close(fd);

        if (fd == m->readfd) {
            m->readfd = 0;
        }else{
            m->writefd = 0;
        }

    }

    return ret;
}

static int
memlink_write(MemLink *m, int fdtype, char *wbuf, int wlen)
{
    int wrlen = 0; 
    int nleft = wlen;
    int ret;
    int fd;
   
    if (fdtype == MEMLINK_READER) {
        fd = m->readfd;
    }else if (fdtype == MEMLINK_WRITER) {
        fd = m->writefd;
    }else{
        return -100;
    }
    DINFO("fd: %d\n", fd);
    if (fd <= 0) {
        DINFO("write fd connect ...\n");
        ret = memlink_connect(m, fdtype);
        DINFO("memlink_connect return: %d\n", ret);
        if (ret < 0)
            return ret;

        if (fdtype == MEMLINK_READER) {
            fd = m->readfd;
        }else{
            fd = m->writefd;
        }
    }

    ret = writen(fd, wbuf, wlen);
    DINFO("writen: %d\n", ret);
    if (ret < 0) {
        close(fd);
        if (fd == m->readfd) {
            m->readfd = 0;
        }else{
            m->writefd = 0;
        }

    }

    return ret;
}

static int
memlink_do_cmd(MemLink *m, int fdtype, char *data, int len, char *retdata, int retlen)
{
    int ret;
    ret = memlink_write(m, fdtype, data, len);
    DINFO("memlink_write ret: %d, len: %d\n", ret, len);
    
    if (ret >= 0 && ret != len) {
        ret = -100;
    }
    
    //char buf[retlen];

    ret = memlink_read(m, fdtype, retdata, retlen);
    DINFO("memlink_read return: %d\n", ret);

    if (ret > 0) {
        int i;
        printf("data: ");
        for (i = 0; i < ret; i++) {
            printf("%x ", (char)retdata[i]);
        }
        printf("\n");
    }

    unsigned short retcode;

    memcpy(&retcode, retdata + sizeof(short), sizeof(short));
    DINFO("retcode: %d\n", retcode);

    return retcode;
}

int
memlink_cmd_dump(MemLink *m)
{
    char data[1024];
    int  len, ret;

    len = cmd_dump_pack(data);
    DINFO("pack dump len: %d\n", len); 

    char retdata[1024];

    return memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
}

int
memlink_cmd_clean(MemLink *m, char *key)
{
    char data[1024];
    int  len;
    //unsigned short dlen;

    len = cmd_clean_pack(data, key);
    //memcpy(&dlen, data, sizeof(short));
    //DINFO("pack clean len: %d, pkg body len: %d\n", len, dlen);
    DINFO("pack clean len: %d\n", len);

    char retdata[1024];
    return memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
}

int 
memlink_cmd_stat(MemLink *m, char *key)
{
    char data[1024];
    int  len;

    len = cmd_stat_pack(data, key);
    DINFO("pack stat len: %d\n", len);

    char retdata[1024];
    int ret = memlink_do_cmd(m, MEMLINK_READER, data, len, retdata, 1024);
    DINFO("memlink_do_cmd: %d\n", ret);
    if (ret <= 0) {
        return ret;
    }

    char msglen;
    int  pos = sizeof(short) + sizeof(short);

    memcpy(&msglen, retdata + pos, sizeof(char));
    if (msglen > 0) {
        pos += msglen;
    }
    
    HashTableStat stat;
    memcpy(&stat, retdata + pos, sizeof(HashTableStat));

    DINFO("stat blocks: %d, data: %d, data_used: %d, mem: %d, mem_used: %d\n", 
                    stat.blocks, stat.data, stat.data_used, stat.mem, stat.mem_used);
    
    return ret;
}

int
memlink_cmd_create(MemLink *m, char *key, int valuelen, char *maskstr)
{
    char data[1024];
    int  len;
    int  masknum = 0;
    unsigned int maskarray[128];   

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("create masknum: %d\n", masknum);
   
    len = cmd_create_pack(data, key, valuelen, masknum, maskarray);
    DINFO("pack create len: %d\n", len);

    printh(data, len); 
    char retdata[1024];
    return memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
}

int
memlink_cmd_del(MemLink *m, char *key, char *value, int valuelen)
{
    char data[1024];
    int  len;

    len = cmd_del_pack(data, key, value, valuelen);
    DINFO("pack del len: %d\n", len);
    
    unsigned short pkglen;
    unsigned char  cmd;

    memcpy(&pkglen, data, sizeof(short));
    memcpy(&cmd, data + sizeof(short), sizeof(char));

    DINFO("pkglen: %d, cmd: %d\n", pkglen, cmd);

    char retdata[1024];
    return memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);
}

int
memlink_cmd_insert(MemLink *m, char *key, char *value, int valuelen, char *maskstr, unsigned int pos)
{
    char data[1024];
    int  len;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("insert mask len: %d\n", masknum);

    len = cmd_insert_pack(data, key, value, valuelen, masknum, maskarray, pos);
    DINFO("pack del len: %d\n", len);

    char retdata[1024];
    return memlink_do_cmd(m, MEMLINK_WRITER, data, len, retdata, 1024);

}

int 
memlink_cmd_update(MemLink *m, char *key, char *value, int valuelen, unsigned int pos)
{
    char data[1024];
    int  len;

    len = cmd_update_pack(data, key, value, valuelen, pos);

    int ret = memlink_write(m, MEMLINK_WRITER, data, len);
    DINFO("update ret: %d, len: %d\n", ret, len);
    
    if (ret >= 0 && ret != len) {
        ret = -100;
    }

    return ret;
}


int
memlink_cmd_mask(MemLink *m, char *key, char *value, int valuelen, char *maskstr)
{
    char data[1024];
    int  len;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("mask mask len:", masknum);

    len = cmd_mask_pack(data, key, value, valuelen, masknum, maskarray);

    int ret = memlink_write(m, MEMLINK_WRITER, data, len);
    DINFO("mask ret: %d, len: %d\n", ret, len);
    
    if (ret >= 0 && ret != len) {
        ret = -100;
    }

    return ret;
}

int
memlink_cmd_tag(MemLink *m, char *key, char *value, int valuelen, int tag)
{
    char data[1024];
    int  len;

    len = cmd_tag_pack(data, key, value, valuelen, tag);

    int ret = memlink_write(m, MEMLINK_WRITER, data, len);
    DINFO("tag ret: %d, len: %d\n", ret, len);
    
    if (ret >= 0 && ret != len) {
        ret = -100;
    }

    return ret;
}

int 
memlink_cmd_range(MemLink *m, char *key, char *maskstr, unsigned int frompos, unsigned int len)
{
    char data[1024];
    int  plen;
    unsigned int maskarray[128];
    int masknum = 0;

    masknum = mask_string2array(maskstr, maskarray);
    DINFO("range mask len:", masknum);

    plen = cmd_range_pack(data, key, masknum, maskarray, frompos, len);

    int ret = memlink_write(m, MEMLINK_WRITER, data, plen);
    DINFO("range ret: %d, len: %d\n", ret, plen);
    
    if (ret >= 0 && ret != plen) {
        ret = -100;
    }

    return ret;
}


void
memlink_close(MemLink *m)
{
    if (m->readfd > 0) {
        close(m->readfd);
        m->readfd = 0;
    }
    if (m->writefd > 0) {
        close(m->writefd);
        m->writefd = 0;
    }

}


void
memlink_destroy(MemLink *m)
{
    memlink_close(m);

    zz_free(m);
}



