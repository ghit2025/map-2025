#include <stdio.h>
#include "manager.h"
#include "common.h"
#include "common_srv.h"
#include "srv_addr_arr.h"

int main(int argc, char *argv[]) {
    if (argc!=2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return 1;
    }
    return 0;
}
