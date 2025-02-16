//
// Created by thomas on 20/05/20.
//

#include <stdint.h>
#include "../xbgp_compliant_api/xbgp_plugin_api.h"
#include "common_rr.h"
#include "../prove_stuffs/prove.h"

/* starting point */
uint64_t decode_cluster_list(args_t *args UNUSED);

PROOF_INSTS(
        uint16_t nondet_u16__verif(void);
        uint8_t nondet_u8(void);

        static uint16_t data_length = 0;

        void *get_arg(unsigned int arg_type) {
            switch (arg_type) {
                case ARG_CODE: {
                    uint8_t *code;
                    code = malloc(sizeof(*code));
                    *code = nondet_u8();
                    return code;
                }
                case ARG_FLAGS: {
                    uint8_t *flags;
                    flags = malloc(sizeof(*flags));
                    *flags = ATTR_OPTIONAL | ATTR_TRANSITIVE;
                    return flags;
                }
                case ARG_DATA: {
                    if (data_length == 0) return NULL;
                    uint8_t *data = malloc(data_length);

                    return data;
                }
                case ARG_LENGTH: {
                    uint16_t *length;
                    if (data_length == 0) {
                        data_length = nondet_u16__verif();
                        p_assume(data_length % 4 == 0);
                    }

                    length = malloc(sizeof(*length));
                    if (!length) return NULL;

                    *length = data_length;
                    return length;
                }
            }

        }

        struct ubpf_peer_info *get_src_peer_info() {
            struct ubpf_peer_info *pf;

            pf = malloc(sizeof(*pf));
            if (!pf) return NULL;

            pf->peer_type = IBGP_SESSION;
            return pf;
        }


#define NEXT_RETURN_VALUE EXIT_SUCCESS
)

#define TIDYING() \
PROOF_INSTS(do {\
 if (code) free(code); \
 if (len) free(len);   \
 if (flags) free(flags); \
 if (data) free(data); \
 if (src_info) free(src_info); \
 if (cluster_list) free(cluster_list); \
} while(0))

uint64_t decode_cluster_list(args_t *args UNUSED) {

    int i;
    struct ubpf_peer_info *src_info = NULL;

    uint8_t *code = NULL;
    uint16_t *len = NULL;
    uint8_t *flags = NULL;
    uint8_t *data = NULL;

    uint32_t *cluster_list = NULL;
    uint32_t *in_cluster_list;

    code = get_arg(ARG_CODE);
    flags = get_arg(ARG_FLAGS);
    data = get_arg(ARG_DATA);
    len = get_arg(ARG_LENGTH);

    src_info = get_src_peer_info();

    if (!src_info || !code || !len || !flags || !data) {
        TIDYING();
        return EXIT_FAILURE;
    }

    if (src_info->peer_type != IBGP_SESSION) {
        TIDYING();
        next(); // don't parse CLUSTER_LIST if originated from eBGP session
    }

    if (*code != CLUSTER_LIST) {
        TIDYING();
        next();
    }

    if (*len % 4 != 0) {
        TIDYING();
        return 0;
    }

    cluster_list = ctx_malloc(*len);
    if (!cluster_list) {
        TIDYING();
        return EXIT_FAILURE;
    }

    in_cluster_list = (uint32_t *) data;

    // T2 doesn't like exact width types
    unsigned int mlen = *len;
    mlen /= 4;

    for (i = 0; i < mlen; i++) {
        cluster_list[i] = ebpf_ntohl(in_cluster_list[i]);
    }

    PROOF_SEAHORN_INSTS(
            p_assert(*len % 4 == 0);
            p_assert(*flags == (ATTR_OPTIONAL | ATTR_TRANSITIVE));
    )


    add_attr(CLUSTER_LIST, *flags, *len, (uint8_t *) cluster_list);
    TIDYING();
    return EXIT_SUCCESS;
}

PROOF_INSTS(
        int main(void) {
            args_t args = {};
            uint64_t ret_val = decode_cluster_list(&args);
            return ret_val;
        }
)