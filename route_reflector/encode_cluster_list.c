//
// Created by thomas on 20/05/20.
//

#include <stdint.h>
#include "../xbgp_compliant_api/xbgp_plugin_api.h"
#include <bytecode_public.h>
#include "common_rr.h"

uint64_t encode_cluster_list(args_t *args UNUSED) {

    uint32_t counter = 0;
    uint8_t *attr_buf;
    uint16_t tot_len = 0;
    uint32_t *cluster_list;
    int i;
    int nb_peer;

    struct path_attribute *attribute;
    attribute = get_attr();

    if (!attribute) return 0;

    struct ubpf_peer_info *to_info;
    to_info = get_peer_info(&nb_peer);

    if (!to_info) {
        ebpf_print("Can't get src and peer info\n");
        return 0;
    }

    if (to_info->peer_type != IBGP_SESSION) {
        next();
    }

    if (attribute->code != CLUSTER_LIST) next();

    tot_len += 2; // Type hdr
    tot_len += attribute->length < 256 ? 1 : 2; // Length hdr
    tot_len += attribute->length;

    attr_buf = ctx_calloc(1, tot_len);
    if (!attr_buf) return 0;

    attr_buf[counter++] = attribute->flags;
    attr_buf[counter++] = attribute->code;

    if (attribute->length < 256) attr_buf[counter++] = (uint8_t) attribute->length;
    else {
        attr_buf[counter] = attribute->length;
        counter += 2;
    }

    cluster_list = (uint32_t *) attribute->data;
    for (i = 0;  i < attribute->length/4; i++) {
        *((uint32_t *)(attr_buf + counter)) = ebpf_htonl(cluster_list[i]);
        counter += 4;
    }

    if(counter != tot_len) {
        ebpf_print("Size missmatch counter %d totlen %d\n", counter, tot_len);
        return 0;
    }

    if (write_to_buffer(attr_buf, counter) == -1) {
        ebpf_print("Write failed\n");
        return 0;
    }
    return counter;
}