#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include <zookeeper/zookeeper.h>

static struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"server", required_argument, NULL, 's'},
	{"root", required_argument, NULL, 'r'},
	{NULL, 0, NULL, 0}
};

static void usage(const char *execfile)
{
	printf("Usage: %s [OPTION...]\n\n", execfile);
	printf("  -s, --server=zkserver       Set zookeeper host, default is \"localhost:2181\"\n");
	printf("  -r, --root=zkroot           Set root znode, default is \"/\"\n");
	printf("  -h, --help                  Show this message\n\n");
}

static void zk_tree_internal(zhandle_t *zk, const char *path, const char *space)
{
	struct String_vector sv;
	int ret = 0;
	int i = 0;
	char abs_path[256] = {'\0'};
	char child_space[64] = {'\0'};

	memset(&sv, 0, sizeof(sv));
	
	if (space[0] == '\0') {
		printf("%s\n", path);
	}

	ret = zoo_wget_children(zk, path, NULL, NULL, &sv);
	if (ret != ZOK || sv.count == 0) {
		deallocate_String_vector(&sv);
		return;
	}

	for (i = 0; i < sv.count - 1; ++i) {
		printf("%s├── %s\n", space, sv.data[i]);

		if (path[0] == '/' && path[1] == '\0') {
			sprintf(abs_path, "/%s", sv.data[i]);
		} else {
			sprintf(abs_path, "%s/%s", path, sv.data[i]);
		}

		sprintf(child_space, "%s│   ", space);
		
		zk_tree_internal(zk, abs_path, child_space);
	}

	printf("%s└── %s\n", space, sv.data[i]);

	if (path[0] == '/' && path[1] == '\0') {
		sprintf(abs_path, "/%s", sv.data[i]);
	} else {
		sprintf(abs_path, "%s/%s", path, sv.data[i]);
	}
	
	sprintf(child_space, "%s    ", space);
	
	zk_tree_internal(zk, abs_path, child_space);

	deallocate_String_vector(&sv);

	return;
}

static void zk_tree(zhandle_t *zk, const char *path)
{
	zk_tree_internal(zk, path, "");
}

int main(int argc, char *argv[])
{
	int c;
	int wait_cnt = 10;
	char *zkserver = "localhost:2181";
	char *zkroot = "/";
	zhandle_t *zk = NULL;

	while ((c = getopt_long(argc, argv, "s:r:h", longopts, NULL)) != EOF ) {
		switch (c) {
			case 's':
				zkserver = optarg;
				break;
			case 'r':
				zkroot = optarg;
				break;
			default:
				usage(argv[0]);
				return 1;
		}
	}

	zoo_set_debug_level(0);

	zk = zookeeper_init(zkserver, NULL, 5000, 0, NULL, 0);
	if (NULL == zk) {
		fprintf(stderr, "[ERROR] zookeeper_init failed!\n");
		return 1;
	}

__WAIT_AGAIN:
	if (ZOO_CONNECTED_STATE != zoo_state(zk)) {
		usleep(200 * 1000);
		if (wait_cnt) {
			wait_cnt--;
			goto __WAIT_AGAIN;
		} else {
			zookeeper_close(zk);
			fprintf(stderr, "[ERROR] zookeeper connect to %s timeout!\n", zkserver);
			return 1;
		}
	}

	zk_tree(zk, zkroot);

	zookeeper_close(zk);

	return 0;
}
