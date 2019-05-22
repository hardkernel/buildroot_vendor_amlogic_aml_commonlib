/*
 * Copyright (C) 2014-2019 Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>

#ifndef SOCKETIPC_H

#define SOCKETIPC_H

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * a lightweight IPC API implemented in a signel header file
 *
 * use DECLARE_IPC_FUNCTION to define the IPC function prototype and IPC stub
 * code i.e. below code will define an IPC call "foo" which takes two parameters
 * typed as int32_t and int64_t:
 *    DECLARE_IPC_FUNCTION(foo,int32_t, int64_t)
 *
 * internally it will declare "call_foo" C API so you can call this function on
 * an opened IPC channel, The stub code is generated when SIPC_IMPLEMENTATION is
 * defined, SIPC_IMPLEMENTATION shall be defined in one of your C code before
 * including this file
 *
 * On the peer side, use EXPORT_IPC_FUNCTION to generate stub code for the IPC
 * call handle:
 *     EXPORT_IPC_FUNCTION(foo,int32_t, int64_t)
 *
 * IPC calls are asynchronous, calls can be make on any thread, the IPC stub
 * code will place an dgram on the socket, the data will transfer to peer, and
 * received by ipc_socket_handle in peer application, it then dispatch the call
 * and call "on_foo"
 *
 */

/*
 * define the IPC type and C type mapping
 * this IPC library is UDP/unix socket based, it is meant to communicate
 * accross different host, so don't use those types whose length depends on ARCH
 * and compiler, such as int, long, use int32_t, int64_t instead
 */
// build-in types
#define SIPC_DEFINE_TYPE_i8 (int),0x01,"\x01"
#define SIPC_DEFINE_TYPE_u8 (int),0x02,"\x02"
#define SIPC_DEFINE_TYPE_i16 (int),0x03,"\x03"
#define SIPC_DEFINE_TYPE_u16 (int),0x04,"\x04"
#define SIPC_DEFINE_TYPE_i32 (int32_t),0x05,"\x05"
#define SIPC_DEFINE_TYPE_u32 (uint32_t),0x06,"\x06"
#define SIPC_DEFINE_TYPE_i64 (int64_t),0x07,"\x07"
#define SIPC_DEFINE_TYPE_u64 (uint64_t),0x08,"\x08"
#define SIPC_DEFINE_TYPE_str (char*),0x09,"\x09"
#define SIPC_DEFINE_TYPE_ptr (void*,int32_t),0x0a,"\x0a"

// type alias
#define SIPC_DEFINE_TYPE_int8_t SIPC_DEFINE_TYPE_i8
#define SIPC_DEFINE_TYPE_char SIPC_DEFINE_TYPE_i8
#define SIPC_DEFINE_TYPE_uint8_t SIPC_DEFINE_TYPE_u8
#define SIPC_DEFINE_TYPE_uchar SIPC_DEFINE_TYPE_u8
#define SIPC_DEFINE_TYPE_byte SIPC_DEFINE_TYPE_u8
#define SIPC_DEFINE_TYPE_int32_t SIPC_DEFINE_TYPE_i32
#define SIPC_DEFINE_TYPE_uint32_t SIPC_DEFINE_TYPE_u32
#define SIPC_DEFINE_TYPE_int64_t SIPC_DEFINE_TYPE_i64
#define SIPC_DEFINE_TYPE_uint64_t SIPC_DEFINE_TYPE_u64


#define MACRO_CAT(a,...) a ## __VA_ARGS__
#define MACRO_CAT2(a,...) MACRO_CAT(a,__VA_ARGS__)
#define MACRO_EMPTY(...)
#define MACRO_CALL(M,...) M(__VA_ARGS__)
#define EXPR(...) __VA_ARGS__

#define MACRO_GET_16(dummy,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,...) _16
#define MACRO_NARG(...) MACRO_GET_16(dummy,##__VA_ARGS__,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)

#define SIPC_TYPE_CTYPE(t) MACRO_CALL(MACRO_GET_16,dummy,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,SIPC_DEFINE_TYPE_##t)
#define SIPC_TYPE_ID(t) MACRO_CALL(MACRO_GET_16,dummy,1,2,3,4,5,6,7,8,9,10,11,12,13,14,SIPC_DEFINE_TYPE_##t)
#define SIPC_TYPE_SID(t) MACRO_CALL(MACRO_GET_16,dummy,1,2,3,4,5,6,7,8,9,10,11,12,13,SIPC_DEFINE_TYPE_##t)

#define MACRO_RECURSE_0(I,E,...)
#define MACRO_RECURSE_1(I,E,_1,...) E(_1)
#define MACRO_RECURSE_2(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_1(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_3(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_2(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_4(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_3(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_5(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_4(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_6(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_5(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_7(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_6(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_8(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_7(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_9(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_8(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_10(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_9(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_11(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_10(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_12(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_11(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_13(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_12(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_14(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_13(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_15(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_14(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_16(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_15(I,E,##__VA_ARGS__)
#define MACRO_RECURSE_17(I,E,_1,...) I(_1,##__VA_ARGS__) MACRO_RECURSE_16(I,E,##__VA_ARGS__)

#define MACRO_MAP(M,...) MACRO_CAT2(MACRO_RECURSE_,MACRO_NARG(__VA_ARGS__))(M,M,##__VA_ARGS__)

#define SIPC_GEN_CTYPE_EXPR1(...) __VA_ARGS__
#define SIPC_GEN_CTYPE_EXPR2(...) __VA_ARGS__
#define SIPC_GEN_CTYPE_E(a,...) SIPC_GEN_CTYPE_EXPR2(SIPC_GEN_CTYPE_EXPR1 SIPC_TYPE_CTYPE(a))
#define SIPC_GEN_CTYPE_I(a,...) SIPC_GEN_CTYPE_EXPR2(SIPC_GEN_CTYPE_EXPR1 SIPC_TYPE_CTYPE(a)),
#define SIPC_EXPAND_CTYPE_LIST(...) MACRO_CAT2(MACRO_RECURSE_,MACRO_NARG(__VA_ARGS__))(SIPC_GEN_CTYPE_I,SIPC_GEN_CTYPE_E,##__VA_ARGS__)

#define SIPC_GEN_ARG_LIST(a,...) ,a MACRO_CAT2(_,MACRO_NARG(__VA_ARGS__))
#define SIPC_GEN_PARAM_LIST(a,...) ,MACRO_CAT2(_,MACRO_NARG(__VA_ARGS__))
#define SIPC_GEN_PARAM_REF_LIST(a,...) ,&MACRO_CAT2(_,MACRO_NARG(__VA_ARGS__))
#define SIPC_GEN_PARAM_STRID(a,...) SIPC_TYPE_SID(a)
#define SIPC_GEN_DECLARE_LIST(a,...) a MACRO_CAT2(_,MACRO_NARG(__VA_ARGS__));
// generate parameter-list: a,b -> , ctype_a _1 ,ctype_b _2
#define SIPC_EXPAND_ARG_LIST(...) MACRO_CALL(MACRO_MAP,SIPC_GEN_ARG_LIST,SIPC_EXPAND_CTYPE_LIST(__VA_ARGS__))
// generate list of declarator: a,b -> , _1 , _2
#define SIPC_EXPAND_PARAM_LIST(...) MACRO_CALL(MACRO_MAP,SIPC_GEN_PARAM_LIST,SIPC_EXPAND_CTYPE_LIST(__VA_ARGS__))
// generate address of declarator list: a,b -> , &_1 , &_2
#define SIPC_EXPAND_PARAM_REF_LIST(...) MACRO_CALL(MACRO_MAP,SIPC_GEN_PARAM_REF_LIST,SIPC_EXPAND_CTYPE_LIST(__VA_ARGS__))
// generate typeid string: a,b -> SIPC_TYPE_SID_a SIPC_TYPE_SID_b
#define SIPC_EXPAND_PARAM_STRID(...) MACRO_MAP(SIPC_GEN_PARAM_STRID,##__VA_ARGS__)
// generate declaration-list: a,b -> SIPC_a _1; SIPC_b _2;
#define SIPC_EXPAND_PARAM_DECLARE_LIST(...) MACRO_CALL(MACRO_MAP,SIPC_GEN_DECLARE_LIST,SIPC_EXPAND_CTYPE_LIST(__VA_ARGS__))

#define SIPC_FUNCTION(n) g_ipc_func_##n

#ifdef SIPC_IMPLEMENTATION
#define DECLARE_IPC_FUNCTION(handle,...) \
    extern SIPC_Function * g_ipc_func_##handle;\
    extern int32_t call_##handle(SIPC_Peer ipc SIPC_EXPAND_ARG_LIST(__VA_ARGS__));\
    static SIPC_Function _ipc_imp_func_##handle = {#handle, (void*)0, "" SIPC_EXPAND_PARAM_STRID(__VA_ARGS__)};\
    SIPC_Function *g_ipc_func_##handle __attribute__((section("ipc_function_import_table"))) = &_ipc_imp_func_##handle;\
    int32_t call_##handle(SIPC_Peer ipc SIPC_EXPAND_ARG_LIST(__VA_ARGS__)) {\
        return ipc_call_fmt(ipc, SIPC_FUNCTION(handle) SIPC_EXPAND_PARAM_LIST(__VA_ARGS__));\
    }
#else
#define DECLARE_IPC_FUNCTION(handle,...)\
    extern SIPC_Function * g_ipc_func_##handle;\
    extern int32_t call_##handle(SIPC_Peer ipc SIPC_EXPAND_ARG_LIST(__VA_ARGS__));
#endif

#define EXPORT_IPC_FUNCTION(handle,...) \
    extern int32_t on_##handle(SIPC_Peer ipc SIPC_EXPAND_ARG_LIST(__VA_ARGS__));\
    static int32_t _ipc_handle_##handle(SIPC_Peer ipc, void * buf, int len) {\
        SIPC_EXPAND_PARAM_DECLARE_LIST(__VA_ARGS__) \
        int32_t r = ipc_upack_args(buf,len,SIPC_EXPAND_PARAM_STRID(__VA_ARGS__) "" SIPC_EXPAND_PARAM_REF_LIST(__VA_ARGS__));\
        if (r >= 0) \
            r = on_##handle(ipc SIPC_EXPAND_PARAM_LIST(__VA_ARGS__)); \
        return r;\
    }\
    static SIPC_Function _ipc_exp_func_##handle = {#handle, _ipc_handle_##handle, "" SIPC_EXPAND_PARAM_STRID(__VA_ARGS__)};\
    SIPC_Function *g_ipc_exp_func_##handle __attribute__((section("ipc_function_export_table"))) = &_ipc_exp_func_##handle;\


typedef struct SIPC_HandleInternal* SIPC_Handle;
typedef struct SIPC_PeerInternal* SIPC_Peer;
typedef struct _SIPC_Function SIPC_Function;

struct _SIPC_Function {
    const char * name;
    int32_t (*handle)(SIPC_Peer , void *, int );
    const char * argfmt;
    int call_id;
    struct _SIPC_Function * next;
};

// user data type can be registered
struct SIPC_DataType {
    int type_id;
    int len;
    int32_t (*pack)(struct SIPC_DataType*, void *, int , va_list *);
    int32_t (*upack)(struct SIPC_DataType*, void *, int , va_list *);
    void (*free)(void *obj);
};

#define SIPC_NO_ERR             0
#define SIPC_ERR_PARAM          -1
#define SIPC_ERR_IO             -2
#define SIPC_ERR_DATA           -3
#define SIPC_ERR_NO_FUNC        -4
#define SIPC_ERR_DUP_FUNC       -5
#define SIPC_ERR_NO_IMPL        -6

// create communication channel
SIPC_Handle ipc_create_channel(const char * bindaddr);
void ipc_stop_channel(SIPC_Handle serv);
// open a peer channel to make IPC call
SIPC_Peer ipc_open_peer(SIPC_Handle serv, const char * remoteaddr);
void ipc_close_peer(SIPC_Peer ipc);
// run the main loop, it will not return
int32_t ipc_run_loop(SIPC_Handle serv);
// exit the most inner loop, return loop level
int32_t ipc_exit_inner_loop(SIPC_Handle serv);
// get the socket fd which can be used in select/poll
// this makes it possible to avoid a dedicate thread to run ipc_run_loop
int ipc_socket_getfd(SIPC_Handle serv);
// call this function if select/poll success on the socket fd
int32_t ipc_socket_handle(SIPC_Handle serv);
// register function calls, the memory of SIPC_Function shall keep valid and untouched until ipc_socket_close
int32_t ipc_register_call(SIPC_Handle serv, SIPC_Function * func);
// call IPC function with parameters
int32_t ipc_call_va(SIPC_Peer ipc, SIPC_Function * func, va_list va);
int32_t ipc_call_fmt(SIPC_Peer ipc, SIPC_Function * func, ...);
// pack and unpack IPC parameters
int32_t ipc_pack_va(void * buf, int len, const char * fmt, va_list va);
int32_t ipc_pack_args(void * buf, int len, const char * fmt, ...);
int32_t ipc_upack_va(void * buf, int len, const char * fmt, va_list va);
int32_t ipc_upack_args(void * buf, int len, const char * fmt, ...);

DECLARE_IPC_FUNCTION(_call,i32,i32,str);
DECLARE_IPC_FUNCTION(_local,i32,ptr);
DECLARE_IPC_FUNCTION(_ping,ptr);
DECLARE_IPC_FUNCTION(_ping_back,ptr);

#ifdef SIPC_IMPLEMENTATION
static int SIPC_LOG_LEVEL = -1;
#define SIPC_LOG(lev,fmt, ...) do {\
    if (lev<SIPC_LOG_LEVEL)\
        printf(fmt "\n", ## __VA_ARGS__);\
    else if (SIPC_LOG_LEVEL == -1) {\
        const char * level = getenv("SIPC_LOG_LEVEL");\
        SIPC_LOG_LEVEL= level?atoi(level):2;\
    }\
} while (0)
#define SIPC_LOG0(fmt, ...) SIPC_LOG(0, fmt, ## __VA_ARGS__)
#define SIPC_LOG1(fmt, ...) SIPC_LOG(1, fmt, ## __VA_ARGS__)
#define SIPC_LOG2(fmt, ...) SIPC_LOG(2, fmt, ## __VA_ARGS__)
#define SIPC_LOG3(fmt, ...) SIPC_LOG(3, fmt, ## __VA_ARGS__)
#define SIPC_LOG4(fmt, ...) SIPC_LOG(4, fmt, ## __VA_ARGS__)

struct SIPC_PeerInternal {
    int32_t client_id;
    SIPC_Handle serv;
    SIPC_Function * func_table;
    struct sockaddr_un peer_addr;
    SIPC_Peer next;
};

struct SIPC_HandleInternal {
    int32_t server_id;
    int sockfd;
    struct SIPC_PeerInternal self;
    SIPC_Function * func_table;
    SIPC_Peer peers;
    int loop_level;
};

static int32_t _ipc_pack_number(struct SIPC_DataType * type, void * buf, int len, va_list * va) {
    if (len < type->len)
        return SIPC_ERR_PARAM;
    uint64_t val64 = (type->len == 8)?va_arg(*va, uint64_t):va_arg(*va, uint32_t);
    uint8_t * p = (uint8_t*)&val64;
    for (int n = type->len; n--; ) {
        ((uint8_t*)buf)[n] = *p++;
    }
    return type->len;
}

static int32_t _ipc_upack_number(struct SIPC_DataType * type, void * buf, int len, va_list *va) {
    if (len < type->len)
        return SIPC_ERR_PARAM;
    uint8_t * pval = va_arg(*va, void*);
    for (int n = type->len; n--; ) {
        *pval++ = ((uint8_t*)buf)[n];
    }
    return type->len;
}

static int32_t _ipc_pack_pointer(struct SIPC_DataType * type, void * buf, int len, va_list *va) {
    void* ptr = va_arg(*va, void*);
    int32_t tlen = type->type_id == SIPC_TYPE_ID(str) ? (ptr ? strlen(ptr) + 1 : 0) : va_arg(*va, int32_t);
    if ((tlen<0) || (len < tlen+4))
        return SIPC_ERR_PARAM;
    ((uint8_t*)buf)[0] = ((uint8_t*)&tlen)[3];
    ((uint8_t*)buf)[1] = ((uint8_t*)&tlen)[2];
    ((uint8_t*)buf)[2] = ((uint8_t*)&tlen)[1];
    ((uint8_t*)buf)[3] = ((uint8_t*)&tlen)[0];
    if (tlen) memcpy((uint8_t *)buf + 4, ptr, tlen);
    return tlen+4;
}

static int32_t _ipc_upack_pointer(struct SIPC_DataType * type, void * buf, int len, va_list *va) {
    int32_t tlen = ((uint32_t)((uint8_t*)buf)[0] << 24) | ((uint32_t)((uint8_t*)buf)[1] << 16) | ((uint32_t)((uint8_t*)buf)[2] << 8) | ((uint32_t)((uint8_t*)buf)[3]);
    if ((tlen<0) && (len < tlen+4))
        return SIPC_ERR_PARAM;
    void ** ptr = va_arg(*va, void**);
    *ptr = tlen ? buf + 4 : NULL;
    if (type->type_id == SIPC_TYPE_ID(ptr)) {
        int32_t * plen = va_arg(*va, int32_t*);
        *plen = tlen;
    }
    return tlen+4;
}

static struct SIPC_DataType g_ipc_buildin_types[] = {
    {0},
    {SIPC_TYPE_ID(i8),1,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(u8),1,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(i16),2,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(u16),2,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(i32),4,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(u32),4,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(i64),8,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(u64),8,_ipc_pack_number,_ipc_upack_number,0},
    {SIPC_TYPE_ID(str),4,_ipc_pack_pointer,_ipc_upack_pointer,0},
    {SIPC_TYPE_ID(ptr),4,_ipc_pack_pointer,_ipc_upack_pointer,0},
};

int32_t ipc_pack_args(void * buf, int len, const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int32_t r = ipc_pack_va(buf,len,fmt,ap);
    va_end(ap);
    return r;
}

int32_t ipc_pack_va(void * buf, int len, const char * fmt, va_list va)
{
    int tlen = 0;
    int32_t ret = 0;
    int ch;
    while ((ch=*fmt++)!='\0') {
        if ((ch >= SIPC_TYPE_ID(i8)) && (ch <= SIPC_TYPE_ID(ptr))) {
            tlen = g_ipc_buildin_types[ch].pack(&g_ipc_buildin_types[ch], buf, len,&va);
        } else {
            SIPC_LOG1("TODO: customer type");
            tlen = SIPC_ERR_NO_IMPL;
        }
        if (tlen <= 0)
            break;
        len -= tlen;
        buf += tlen;
        ret += tlen;
    }
    return ret;
}

int32_t ipc_upack_args(void * buf, int len, const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int32_t r = ipc_upack_va(buf,len,fmt,ap);
    va_end(ap);
    return r;
}

int32_t ipc_upack_va(void * buf, int len, const char * fmt, va_list va)
{
    int ch;
    int tlen = 0;
    int32_t ret = 0;
    while ((ch=*fmt++)!='\0') {
        if ((ch >= SIPC_TYPE_ID(i8)) && (ch <= SIPC_TYPE_ID(ptr))) {
            tlen = g_ipc_buildin_types[ch].upack(&g_ipc_buildin_types[ch], buf, len,&va);
        } else {
            SIPC_LOG1("TODO: customer type");
            tlen = SIPC_ERR_NO_IMPL;
        }
        if (tlen <= 0)
            break;
        len -= tlen;
        buf += tlen;
        ret += tlen;
    }
    return ret;
}

SIPC_Handle ipc_create_channel(const char * bindaddr)
{
    int unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (unix_socket < 0) {
        SIPC_LOG1("failed to open socket %d %s", errno, strerror(errno));
        goto error;
    }
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    if (bindaddr) {
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", bindaddr);
        unlink(bindaddr);
        if (bind(unix_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            SIPC_LOG1("bind to %s fail %d %s", bindaddr, errno, strerror(errno));
            goto error;
        }
    }
    struct SIPC_HandleInternal * serv = malloc(sizeof(*serv));
    memset(serv, 0, sizeof(*serv));
    memcpy(&serv->self.peer_addr, &addr, sizeof(addr));
    serv->sockfd = unix_socket;
    serv->peers = NULL;
    serv->func_table = NULL;
    serv->self.serv = serv;

    return serv;

error:
    if (unix_socket >= 0) {
        close(unix_socket);
    }
    return NULL;
}

SIPC_Peer ipc_open_peer(SIPC_Handle serv, const char * remoteaddr)
{
    SIPC_Peer ipc = serv->peers;
    for (; ipc; ipc=ipc->next) {
        if (strcmp(ipc->peer_addr.sun_path, remoteaddr) == 0)
            return ipc;
    }
    ipc = malloc(sizeof(*ipc));
    memset(ipc, 0, sizeof(*ipc));
    ipc->serv = serv;
    ipc->next = serv->peers;
    ipc->peer_addr.sun_family = AF_UNIX;
    snprintf(ipc->peer_addr.sun_path, sizeof(ipc->peer_addr.sun_path), "%s", remoteaddr);
    serv->peers = ipc;
    return ipc;
}

void ipc_close_peer(SIPC_Peer ipc)
{
    SIPC_Peer ch = ipc->serv->peers;
    if (ch == ipc)
        ipc->serv->peers = ch->next;
    else if (ch) {
        for (; ch->next; ch=ch->next) {
            if (ch->next == ipc) {
                ch->next = ipc->next;
                break;
            }
        }
    }
    free(ipc);
}

void ipc_stop_channel(SIPC_Handle serv)
{
    if (serv->sockfd >= 0) {
        close(serv->sockfd);
        serv->sockfd = -1;
    }
    while (serv->peers) {
        SIPC_Peer ipc = serv->peers;
        serv->peers = ipc->next;
        ipc_close_peer(ipc);
    }
}

int32_t ipc_run_loop(SIPC_Handle serv)
{
    int cur_loop = serv->loop_level++;
    while (cur_loop < serv->loop_level) {
        int r = ipc_socket_handle(serv);
        (void)r;
    }
    return 0;
}

int32_t ipc_exit_inner_loop(SIPC_Handle serv)
{
    serv->loop_level--;
    call__local(&serv->self,0,NULL,0);
    return 0;
}

int ipc_socket_getfd(SIPC_Handle serv)
{
    return serv->sockfd;
}

// clientid,callid,name
int32_t ipc_socket_handle(SIPC_Handle serv)
{
    char buf[1024*8];
    struct SIPC_PeerInternal tempChannel;
    socklen_t addrlen = sizeof(tempChannel.peer_addr);
    int len = recvfrom(serv->sockfd, buf, sizeof(buf), MSG_NOSIGNAL, (struct sockaddr *)&tempChannel.peer_addr, &addrlen);
    int32_t client_id;
    int32_t call_id;
    char * name;
    int len1 = ipc_upack_args(buf, len, SIPC_FUNCTION(_call)->argfmt, &client_id, &call_id, &name);
    if (len1 <= 0) {
        SIPC_LOG2("data format error");
        return SIPC_ERR_DATA;
    }
    SIPC_LOG3("received call %s %d bytes from %s", name, len, tempChannel.peer_addr.sun_path);
    if (SIPC_LOG_LEVEL > 3) {
        int i;
        char msg[len*3 + 1], *p = msg;
        for (i=0; i<len; ++i) {
            p += sprintf(p, "%02x ", (uint8_t)buf[i]);
        }
        SIPC_LOG4("%s",msg);
    }
    SIPC_Peer ipc = NULL;
    if (client_id == 0) {
        ipc = &tempChannel;
        ipc->serv = serv;
        ipc->next = NULL;
        ipc->client_id = 0;
        ipc->func_table = NULL;
    } else {
        for (ipc=serv->peers; ipc; ipc=ipc->next) {
            if (ipc->client_id == client_id)
                break;
        }
    }
    if (ipc == NULL) {
        SIPC_LOG2("client_id %d not found", client_id);
    }
    extern SIPC_Function *__start_ipc_function_export_table;
    extern SIPC_Function *__stop_ipc_function_export_table;
    //extern SIPC_Function *__start_ipc_function_import_table;
    //extern SIPC_Function *__stop_ipc_function_import_table;
    SIPC_Function * f = NULL;
    SIPC_Function ** ppfunc = &__start_ipc_function_export_table;
    for (;ppfunc<&__stop_ipc_function_export_table;++ppfunc) {
        if (strcmp((*ppfunc)->name, name) == 0) {
            f = *ppfunc;
            break;
        }
    }
    if (f == NULL) {
        for (f=serv->func_table; f; f=f->next) {
            if (strcmp(f->name, name) == 0) {
                break;
            }
        }
    }
    if (f == NULL) {
        SIPC_LOG2("can't find function %s", (char*)name);
        return SIPC_ERR_NO_FUNC;
    }
    return f->handle(ipc, buf+len1, len-len1);
}

int32_t ipc_register_call(SIPC_Handle serv, SIPC_Function * func)
{
    SIPC_Function *f;
    for (f=serv->func_table; f != NULL; f = f->next) {
        if ((f == func) || (strcmp(f->name, func->name)==0)) {
            SIPC_LOG1("try to register IPC function %s multiple time, %p %p\n", func->name, f, func);
            return SIPC_ERR_DUP_FUNC;
        }
    }
    func->next = serv->func_table;
    serv->func_table = func;
    return 0;
}

int32_t ipc_call_va(SIPC_Peer ipc, SIPC_Function * func, va_list va)
{
    char buf[1024*8];
    int len = sizeof(buf);
    int len1 = ipc_pack_args(buf, len, SIPC_FUNCTION(_call)->argfmt, ipc->client_id, func->call_id, func->name);
    if (len1 <= 0) {
        SIPC_LOG2("call ipc_pack_args header failed");
        return len1;
    }
    int len2 = ipc_pack_va(buf+len1, len-len1, func->argfmt, va);
    if (len2 < 0) {
        SIPC_LOG2("call ipc_pack_args params failed");
        return len2;
    }
    int wlen = sendto(ipc->serv->sockfd, buf, len1+len2, MSG_NOSIGNAL, (struct sockaddr *)&ipc->peer_addr, sizeof(ipc->peer_addr));
    SIPC_LOG3("call %s, send %d bytes to %s, return %d", func->name, len1+len2, ipc->peer_addr.sun_path, wlen);
    if (SIPC_LOG_LEVEL > 3) {
        int i = len1+len2;
        char msg[i*3 + 1], *p = msg;
        for (i=0; i<len1+len2; ++i) {
            p += sprintf(p, "%02x ", (uint8_t)buf[i]);
        }
        SIPC_LOG4("%s",msg);
    }
    if (wlen != len1+len2) {
        SIPC_LOG2("send data fail %d != %d + %d, err %d %s", wlen, len1, len2, errno, strerror(errno));
        return SIPC_ERR_IO;
    }
    return 0;
}

int32_t ipc_call_fmt(SIPC_Peer ipc, SIPC_Function * func, ...) {
    va_list ap;
    va_start(ap, func);
    int32_t r = ipc_call_va(ipc,func,ap);
    va_end(ap);
    return r;
}

EXPORT_IPC_FUNCTION(_local,i32,ptr);
EXPORT_IPC_FUNCTION(_ping,ptr);
EXPORT_IPC_FUNCTION(_ping_back,ptr);

int32_t on__ping(SIPC_Peer ipc, void * data, int32_t len)
{
    SIPC_LOG3("client %p ping %p %d", ipc, data, len);
    call__ping_back(ipc, data, len);
    return 0;
}

int32_t on__ping_back(SIPC_Peer ipc, void * data, int32_t len)
{
    SIPC_LOG3("client %p ping back %p %d", ipc, data, len);
    return 0;
}

int32_t on__local(SIPC_Peer ipc, int32_t cmd, void * data, int32_t len)
{
    SIPC_LOG3("client %p call local %d %p %d", ipc, cmd, data, len);
    return 0;
}



#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: SOCKETIPC_H */
