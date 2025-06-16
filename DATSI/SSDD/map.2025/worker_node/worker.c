#include <stdio.h>
#include "worker.h"
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "common_cln.h"

int main(int argc, char *argv[]) {
    if (argc!=3) {
        fprintf(stderr, "Usage: %s manager_host manager_port\n", argv[0]);
        return 1;
    }
    return 0;
}

