// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <signal.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include <errno.h>

#include "ubpf.h"
#include "bpfmap.h"
#include "ebpf_fntypes.h"

#include "Header.pb-c.h"
#include "Hello.pb-c.h"
#include "Install.pb-c.h"
#include "Table.pb-c.h"
#include "Packet.pb-c.h"
#include "Notify.pb-c.h"

#include "agent.h"

#ifndef likely
    #define likely(x)        __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
    #define unlikely(x)        __builtin_expect(!!(x), 0)
#endif

static sig_atomic_t sigint = 0;

typedef int (*handler)(void* buffer, Header *header);

struct agent {
    int fd;
    ubpf_jit_fn *ubpf_fn;
    tx_packet_fn transmit;

    struct agent_options *options;
} agent;

struct ubpf_vm *vm;

void send_hello()
{
    Header header = HEADER__INIT;
    Hello hello = HELLO__INIT;

    header.type = HEADER__TYPE__HELLO;
    hello.version = 1;
    hello.dpid = agent.options->dpid;

    //
    int packet_len = hello__get_packed_size(&hello);
    header.length = packet_len;

    int header_len = header__get_packed_size(&header);

    void *buf = malloc(header_len + packet_len);
    header__pack(&header, buf);
    hello__pack(&hello, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);
    free(buf);
}

int recv_hello(void *buffer, Header *header)
{
    Hello *hello;

    hello = hello__unpack(NULL, header->length, buffer);
    int len = hello__get_packed_size(hello);
    hello__free_unpacked(hello, NULL);

    return len;
}

int recv_install(void *buffer, Header *header)
{
    InstallRequest *request;

    request = install_request__unpack(NULL, header->length, buffer);
    int len = install_request__get_packed_size(request);

    //
    int err;
    char *errmsg;
    err = ubpf_load_elf(vm, request->elf.data, request->elf.len, &errmsg);

    if (err != 0) {
        printf("Error message: %s\n", errmsg);
        free(errmsg);
    }

    ubpf_jit_fn ebpfprog = ubpf_compile(vm, &errmsg);

    if (ebpfprog == NULL) {
        printf("Error JIT %s\n", errmsg);
        free(errmsg);
    }

    *(agent.ubpf_fn) = ebpfprog;

    //TODO should send a InstallReply

    //
    install_request__free_unpacked(request, NULL);
    return len;
}

int recv_tables_list_request(void *buffer, Header *header) {
    TablesListRequest *request;
    request = tables_list_request__unpack(NULL, header->length, buffer);
    int len = tables_list_request__get_packed_size(request);
    // TODO should free pkt memory

    // Reply
    TableDefinition **entries;
    entries = malloc(sizeof(TableDefinition *) * TABLE_MAX_ENTRIES); // NOTE: allocate TABLE_MAX_ENTRIES as we don't know the number of entries in the table

    //
    int n_entries = 0;
    char table_name[32] = {0};
    struct table_entry *tab_entry;

    int tables = ubpf_get_tables(vm);
    while (bpf_get_next_key(tables, table_name, table_name) == 0) {
        bpf_lookup_elem(tables, table_name, &tab_entry);

        entries[n_entries] = malloc(sizeof(TableDefinition));
        table_definition__init(entries[n_entries]);

        // not the best way, copy twice the table name, on lookup and to insert the entry
        entries[n_entries]->table_name = malloc(strlen(table_name) + 1);
        strcpy(entries[n_entries]->table_name, table_name);

        entries[n_entries]->table_type = tab_entry->type;
        entries[n_entries]->key_size = tab_entry->key_size;
        entries[n_entries]->value_size = tab_entry->value_size;
        entries[n_entries]->max_entries = tab_entry->max_entries;

        n_entries++;
    }

    TablesListReply reply = TABLES_LIST_REPLY__INIT;
    reply.n_entries = n_entries;
    reply.entries = entries;

    int packet_len = tables_list_reply__get_packed_size(&reply);

    Header replyHeader = HEADER__INIT;
    replyHeader.type = HEADER__TYPE__TABLES_LIST_REPLY;
    replyHeader.length = packet_len;

    int header_len = header__get_packed_size(&replyHeader);

    void *buf = malloc(header_len + packet_len);
    header__pack(&replyHeader, buf);
    tables_list_reply__pack(&reply, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    // house keeping
    free(buf);
    int i;
    for (i = 0; i < n_entries; i++) {
        free(entries[i]->table_name);
        free(entries[i]);
    }

    free(entries);

    return len;
}

int recv_table_list_request(void *buffer, Header *header) {
    TableListRequest *request;
    request = table_list_request__unpack(NULL, header->length, buffer);
    int len = table_list_request__get_packed_size(request);

    // TODO should free pkt memory

    // Reply
    TableListReply reply = TABLE_LIST_REPLY__INIT;

    //
    char table_name[32] = {0};
    strncpy(table_name, request->table_name, 31);
    struct table_entry *tab_entry;

    int tables = ubpf_get_tables(vm);
    int ret = bpf_lookup_elem(tables, table_name, &tab_entry);

    TableDefinition tableEntry = TABLE_DEFINITION__INIT;

    if (ret == -1) {
        reply.status = TABLE_STATUS__TABLE_NOT_FOUND;
    } else {
        reply.status = TABLE_STATUS__SUCCESS;

        tableEntry.table_name = request->table_name;
        tableEntry.table_type = tab_entry->type;
        tableEntry.key_size = tab_entry->key_size;
        tableEntry.value_size = tab_entry->value_size;
        tableEntry.max_entries = tab_entry->max_entries;

        reply.entry = &tableEntry;

        int n_items = 0;
        int item_size;
        unsigned char *items;

        if (tab_entry->type == BPF_MAP_TYPE_HASH) {
            item_size = tab_entry->key_size + tab_entry->value_size;
            items = calloc(tab_entry->max_entries, item_size);

            unsigned char *key = items;
            unsigned char *next_key = items;
            unsigned char *value;

            while (bpf_get_next_key(tab_entry->fd, key, next_key) == 0) {
                bpf_lookup_elem(tab_entry->fd, next_key, &value);
                memcpy(next_key + tab_entry->key_size, value, tab_entry->value_size);

                n_items++;
                key = next_key;
                next_key = items + n_items * item_size;
            }
        }

        else if (tab_entry->type == BPF_MAP_TYPE_ARRAY) {
            uint32_t key = 0;
            n_items = tab_entry->max_entries;
            item_size = tab_entry->value_size;

            void *data;
            items = malloc(n_items * item_size);
            bpf_lookup_elem(tab_entry->fd, &key, &data);
            memcpy(items, data, n_items * item_size);
        }

        reply.n_items = n_items;
        reply.has_n_items = 1;

        reply.items.len = n_items * item_size;
        reply.items.data = items;
        reply.has_items = 1; // Why are optional fields not working?

        // TODO housekeeping
    }

    int packet_len = table_list_reply__get_packed_size(&reply);

    Header replyHeader = HEADER__INIT;
    replyHeader.type = HEADER__TYPE__TABLE_LIST_REPLY;
    replyHeader.length = packet_len;

    int header_len = header__get_packed_size(&replyHeader);

    void *buf = malloc(header_len + packet_len);
    header__pack(&replyHeader, buf);
    table_list_reply__pack(&reply, buf+header_len);
    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    free(buf);
    free(reply.items.data);

    return len;
}

int recv_table_entry_get_request(void *buffer, Header *header) {
    TableEntryGetRequest *request;
    request = table_entry_get_request__unpack(NULL, header->length, buffer);
    int len = table_entry_get_request__get_packed_size(request);

    //
    TableEntryGetReply reply = TABLE_ENTRY_GET_REPLY__INIT;

    char table_name[32] = {0};
    strncpy(table_name, request->table_name, 31);
    struct table_entry *tab_entry;
    int tables = ubpf_get_tables(vm);
    int ret = bpf_lookup_elem(tables, table_name, &tab_entry);

    if (ret == -1) {
        reply.status = TABLE_STATUS__TABLE_NOT_FOUND;
    } else {
        reply.has_key = 1;
        reply.key = request->key;
        reply.value.len = tab_entry->value_size;
        reply.value.data = malloc(reply.value.len);

        ret = bpf_lookup_elem(tab_entry->fd, request->key.data, &reply.value.data);
        if (ret == -1) {
            reply.status = TABLE_STATUS__ENTRY_NOT_FOUND;
        } else {
            reply.status = TABLE_STATUS__SUCCESS;
            reply.has_value = 1;
        }
    }

    int packet_len = table_entry_get_reply__get_packed_size(&reply);
    Header replyHeader = HEADER__INIT;
    replyHeader.type = HEADER__TYPE__TABLE_ENTRY_GET_REPLY;
    replyHeader.length = packet_len;

    int header_len = header__get_packed_size(&replyHeader);

    void *buf = malloc(header_len + packet_len);
    header__pack(&replyHeader, buf);
    table_entry_get_reply__pack(&reply, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    free(reply.value.data);
    free(buf);

    return len;
}

int recv_table_entry_insert_request(void *buffer, Header *header) {
    TableEntryInsertRequest *request;
    request = table_entry_insert_request__unpack(NULL, header->length, buffer);
    int len = table_entry_insert_request__get_packed_size(request);


    //
    TableEntryInsertReply reply = TABLE_ENTRY_INSERT_REPLY__INIT;

    char table_name[32] = {0};
    strncpy(table_name, request->table_name, 31);
    struct table_entry *tab_entry;
    int tables = ubpf_get_tables(vm);
    int ret = bpf_lookup_elem(tables, table_name, &tab_entry);

    if (ret == -1) {
        reply.status = TABLE_STATUS__TABLE_NOT_FOUND;
    } else {
        ret = bpf_update_elem(tab_entry->fd, request->key.data, request->value.data, 0); // flags not handled for now
        reply.status = TABLE_STATUS__SUCCESS;
        // NOTE: how to handle the insert return code?
    }

    int packet_len = table_entry_insert_reply__get_packed_size(&reply);
    Header replyHeader = HEADER__INIT;
    replyHeader.type = HEADER__TYPE__TABLE_ENTRY_INSERT_REPLY;
    replyHeader.length = packet_len;

    int header_len = header__get_packed_size(&replyHeader);

    void *buf = malloc(header_len + packet_len);
    header__pack(&replyHeader, buf);
    table_entry_insert_reply__pack(&reply, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    free(buf);

    return len;
}

int recv_table_entry_delete_request(void *buffer, Header *header) {
    TableEntryDeleteRequest *request;
    request = table_entry_delete_request__unpack(NULL, header->length, buffer);
    int len = table_entry_delete_request__get_packed_size(request);

    //
    TableEntryDeleteReply reply = TABLE_ENTRY_DELETE_REPLY__INIT;

    char table_name[32] = {0};
    strncpy(table_name, request->table_name, 31);
    struct table_entry *tab_entry;
    int tables = ubpf_get_tables(vm);
    int ret = bpf_lookup_elem(tables, table_name, &tab_entry);

    if (ret == -1) {
        reply.status = TABLE_STATUS__TABLE_NOT_FOUND;
    } else {
        ret = bpf_delete_elem(tab_entry->fd, request->key.data);
        if (ret == -1) {
            reply.status = TABLE_STATUS__ENTRY_NOT_FOUND;
        } else {
            reply.status = TABLE_STATUS__SUCCESS;
        }
    }

    int packet_len = table_entry_delete_reply__get_packed_size(&reply);
    Header replyHeader = HEADER__INIT;
    replyHeader.type = HEADER__TYPE__TABLE_ENTRY_DELETE_REPLY;
    replyHeader.length = packet_len;

    int header_len = header__get_packed_size(&replyHeader);

    void *buf = malloc(header_len + packet_len);
    header__pack(&replyHeader, buf);
    table_entry_delete_reply__pack(&reply, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    free(buf);

    return len;
}

int recv_packet_out(void *buffer, Header *header) {
    PacketOut *request;
    request = packet_out__unpack(NULL, header->length, buffer);
    int len = packet_out__get_packed_size(request);

    agent.transmit(request->data.data, request->data.len, request->out_port, 1);

    return len;
}

const handler handlers[] = {
    [HEADER__TYPE__HELLO] = recv_hello,
    [HEADER__TYPE__INSTALL_REQUEST] = recv_install,
    [HEADER__TYPE__TABLES_LIST_REQUEST] = recv_tables_list_request,
    [HEADER__TYPE__TABLE_LIST_REQUEST] = recv_table_list_request,
    [HEADER__TYPE__TABLE_ENTRY_GET_REQUEST] = recv_table_entry_get_request,
    [HEADER__TYPE__TABLE_ENTRY_INSERT_REQUEST] = recv_table_entry_insert_request,
    [HEADER__TYPE__TABLE_ENTRY_DELETE_REQUEST] = recv_table_entry_delete_request,
    [HEADER__TYPE__PACKET_OUT] = recv_packet_out,
};

int agent_packetin(void *pkt, int len) {
    PacketIn reply = PACKET_IN__INIT;
    Header replyHeader = HEADER__INIT;

    int header_len = header__get_packed_size(&replyHeader);

    reply.data.len = len;
    reply.data.data = pkt;

    int packet_len = packet_in__get_packed_size(&reply);
    replyHeader.type = HEADER__TYPE__PACKET_IN;
    replyHeader.length = packet_len;

    void *buf = malloc(header_len + packet_len);
    header__pack(&replyHeader, buf);
    packet_in__pack(&reply, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    return 0;
}

uint64_t bpf_debug(uint64_t r1, uint64_t r2, uint64_t r3, uint64_t r4, uint64_t r5)
{
    char *s = (char *)r1;
    printf("BPF_DEBUG: %s\n", s);
    return 0;
}

uint64_t bpf_notify(uint64_t r1, uint64_t r2, uint64_t r3, uint64_t r4, uint64_t r5)
{
    int id = (int)r1;
    void *payload = (void *)r2;
    int len = (int)r3;

    Notify notify = NOTIFY__INIT;
    Header header = HEADER__INIT;

    notify.id = id;
    notify.data.data = payload;
    notify.data.len = len;

    int packet_len = notify__get_packed_size(&notify);
    header.type = HEADER__TYPE__NOTIFY;
    header.length = packet_len;

    int header_len = header__get_packed_size(&header);

    void *buf = malloc(header_len + packet_len);
    header__pack(&header, buf);
    notify__pack(&notify, buf+header_len);

    send(agent.fd, buf, header_len + packet_len, MSG_NOSIGNAL);

    return 0;
}

/* (Re-)declare exposed functions to check their signatures.
   Note: the original functions do not match! */
BPF_FNCHECK(bzero);
BPF_FNCHECK(bcopy);
static BPF_FNCHECK(bsalt);
static BPF_FNCHECK(bpf_trace);
static BPF_FNCHECK(digest_init);
static BPF_FNCHECK(digest_update);
static BPF_FNCHECK(digest_final);
static BPF_FNCHECK(crypt_verify);

void *agent_task()
{
    //
    uint8_t buf[65536]; // TODO should have a proper buffer that wraps around and expand if the message is bigger than this
    struct sockaddr_in saddr;

    //
    char *controller_address, *controller_ip, *controller_port;
    controller_address = controller_port = strdup(agent.options->controller);
    controller_ip = strsep(&controller_port, ":");

    //
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(controller_port));
    if (inet_pton(AF_INET, controller_address, &saddr.sin_addr) <= 0) {
        perror("error resolving server address");
        pthread_exit(NULL);
    }

    //
    vm = ubpf_create();


    // Register the map functions
    ubpf_register(vm, 1, "bpf_map_lookup_elem", bpf_lookup_elem);
    ubpf_register(vm, 2, "bpf_map_update_elem", bpf_update_elem);
    ubpf_register(vm, 3, "bpf_map_delete_elem", bpf_delete_elem);
    BPF_REGISTER(vm, digest_init);
    BPF_REGISTER(vm, digest_update);
    BPF_REGISTER(vm, digest_final);
    BPF_REGISTER(vm, crypt_verify);
    BPF_REGISTER(vm, bzero);
    BPF_REGISTER(vm, bcopy);
    BPF_REGISTER(vm, bsalt);
    BPF_REGISTER(vm, bpf_trace);
    ubpf_register(vm, 31, "bpf_notify", bpf_notify);
    ubpf_register(vm, 32, "bpf_debug", bpf_debug);

    while (likely(!sigint)) {
        // Connect to the controller
        agent.fd = socket(AF_INET, SOCK_STREAM, 0);

        if (agent.fd >= 0) {
            if (connect(agent.fd, (struct sockaddr *)&saddr, sizeof(saddr)) == 0) {
                printf("connection established!\n");

                // CONFIGURATION
                send_hello();

                // MAIN Event Loop
                Header *header;
                while (likely(!sigint)) {
                    // Recv can get multiple headers + payload
                    int offset = 0;
                    int len = recv(agent.fd, buf, sizeof(buf), 0);
                    // printf("received length %d\n", len);

                    if (len <= 0) {
                        break;
                    }

                    // Not great if we don't receive a full header + payload in one go
                    while (len - offset >= 10) {
                        // Read the packet header
                        header = header__unpack(NULL, 10, buf + offset); // Need to harcode the length at unpack is greedy
                        offset += 10;

                        if (header != NULL) {
                            // printf("header type: %d  length: %d\n", header->type, header->length);

                            handler h = handlers[header->type];
                            // printf("handler %p\n", h);
                            offset += h(buf+offset, header);

                            header__free_unpacked(header, NULL);
                        } else {
                            printf("error unpacking incoming message\n");
                            break;
                        }

                        // printf("\n");
                    }
                }

                // TEARDOWN
                close(agent.fd);
            }
        }

        perror("unable to connect to the controller");
        sleep(5);
    }

    pthread_exit(NULL);
}

int agent_start(ubpf_jit_fn *ubpf_fn, tx_packet_fn tx_fn, struct agent_options *opts)
{
    int err;
    pthread_t agent_thread;

    agent.ubpf_fn = ubpf_fn;
    agent.transmit = tx_fn;
    agent.options = opts;

    err = pthread_create(&agent_thread, NULL, agent_task, NULL);
    return err;
}

int agent_stop(void)
{
    sigint = 1;
    return sigint;
}

static void bsalt(void *base, size_t len)
{
    unsigned char *p = base;
    while (len > 0) {
        *p = rand() / (RAND_MAX + 1.0) * 256;
        len--;
    }
}

static void bpf_trace(const char *fn, uint64_t line, uint64_t value)
{
    fprintf(stderr, "BPF dpid=%u: %s:%" PRIu64 " => %" PRIu64
            " (0x%" PRIx64 ")\n",
            (unsigned) agent.options->dpid, fn, line, value, value);
    fflush(stderr);
}

#include <openssl/sha.h>
#include "ebpf_digest.h"

union digest_ctxts {
    digest_ctxt_t generic;
    digest_ctxt_SHA256_t SHA256;
};

static void digest_init(void *vctxt, const void *params)
{
    union digest_ctxts *ctxt = vctxt;
    switch (ctxt->generic.type) {
    default:
        abort();
        break;

    case digest_SHA256:
        if (SHA256_Init(&ctxt->SHA256.state) != 1)
            abort();
        break;
    }
}

static void digest_update(void *vctxt, const void *base, size_t len)
{
    union digest_ctxts *ctxt = vctxt;
    switch (ctxt->generic.type) {
    default:
        abort();
        break;

    case digest_SHA256:
        if (SHA256_Update(&ctxt->SHA256.state, base, len) != 1)
            abort();
        break;
    }
}

static void digest_final(void *vctxt, void *res)
{
    union digest_ctxts *ctxt = vctxt;
    switch (ctxt->generic.type) {
    default:
        abort();
        break;

    case digest_SHA256:
        if (SHA256_Final(res, &ctxt->SHA256.state) != 1)
            abort();
        break;
    }
}

#include "ebpf_crypt.h"

union crypt_ctxts {
    crypt_ctxt_t generic;
};

static int crypt_verify(void *vctxt,
                        const unsigned char *hash,
                        size_t hashlen,
                        const unsigned char *sig,
                        const unsigned char *pubkey)
{
    union crypt_ctxts *ctxt = vctxt;
    switch (ctxt->generic.type) {
    default:
        abort();
        break;

    case crypt_SEPC256K1:
        return uECC_verify(pubkey, hash, hashlen, sig, uECC_secp256k1());
    }
    return -1;
}
