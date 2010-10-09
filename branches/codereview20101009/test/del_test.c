#include <stdio.h>
#include <stdlib.h>
#include <memlink_client.h>
#include "logfile.h"

int main()
{
	MemLink	*m;
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif

	
	m = memlink_create("127.0.0.1", 11001, 11002, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}
	
	int  ret;
	char buf[64];
	
	sprintf(buf, "haha");
	ret = memlink_cmd_create(m, buf, 6, "4:3:2");
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d\n", buf, ret);
		return -2;
	}

	int i;
	char *maskstr = "8:3:1";
	char val[64];

	for (i = 0; i < 100; i++) {
		sprintf(val, "%06d", i);

		ret = memlink_cmd_insert(m, buf, val, strlen(val), maskstr, i * 3);
		if (ret != MEMLINK_OK) {
			DERROR("insert error, key:%s, value:%s, mask:%s, i:%d\n", buf, val, maskstr, i);
			return -3;
		}
	}
	
	char buf2[64];

	ret = memlink_cmd_del(m, buf, "xxxx", 4);
	if (ret != MEMLINK_ERR_NOVAL) {
		DERROR("del error, must novalue, key:%s, val:%s\n", buf, "xxxx");
		return -4;
	}

	for (i = 0; i < 100; i++) {
		sprintf(val, "%06d", i);
		
		ret = memlink_cmd_del(m, buf, val, strlen(val));
		if (ret != MEMLINK_OK) {
			DERROR("del error, key:%s, val:%s\n", buf, val);
			return -5;
		}

		MemLinkStat	stat;

		ret = memlink_cmd_stat(m, buf, &stat);
		if (ret != MEMLINK_OK) {
			DERROR("stat error, key:%s\n", buf);
		}
	
		if (stat.data_used != 100 - i - 1) {
			DERROR("del not remove item! key:%s, val:%s\n", buf, val);
		}
	}

	memlink_destroy(m);

	return 0;
}
