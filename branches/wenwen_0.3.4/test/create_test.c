#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memlink_client.h>
#include "logfile.h"
#include "test.h"

int main()
{
	MemLink	*m;
#ifdef DEBUG
	logfile_create("stdout", 3);
#endif
	m = memlink_create("127.0.0.1", MEMLINK_READ_PORT, MEMLINK_WRITE_PORT, 30);
	if (NULL == m) {
		DERROR("memlink_create error!\n");
		return -1;
	}

	int  ret;
	char key[32];
	
	int nodenum = 100;
	int i;
///////////创建100个不同key的hashnode
	for( i = 0 ; i < nodenum; i++) 
	{
		sprintf(key, "haha%d", i);
		ret = memlink_cmd_create_list(m, key, 6, "4:3:1");
		
		if (ret != MEMLINK_OK) {
			DERROR("memlink_cmd_create %s error: %d\n", key, ret);
			return -2;
		}
	}
	
	ret = memlink_cmd_create_list(m, key, 6, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d\n", key, ret);
		return -3;
	}

	strcpy(key, "haha1111");
	ret = memlink_cmd_create_list(m, key, -1, "4:3:1");
	if (ret == MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d\n", key, ret);
		return -3;
	}

	strcpy(key, "haha2222");
	ret = memlink_cmd_create_list(m, key, 12, "4:3:21474");
	if (ret == MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d, mask=%s\n", key, ret, "4:3:21474");
		return -3;
	}

	strcpy(key, "haha3333");
	ret = memlink_cmd_create_list(m, key, 12, "");
	if (ret != MEMLINK_OK) {
		DERROR("memlink_cmd_create %s error: %d, mask=%s\n", key, ret, "");
		return -3;
	}
	
	memlink_destroy(m);

	return 0;
}
