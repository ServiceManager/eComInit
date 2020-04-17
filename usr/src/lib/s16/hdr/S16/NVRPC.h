/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/SYSTEMXVI.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2020 David MacKay.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef S16NEWRPC_H_
#define S16NEWRPC_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

#include <nv.h>
#include <ucl.h>

#include "S16/List.h"

    typedef intptr_t boolptr_t;
    typedef intptr_t fdptr_t;

    typedef enum
    {
        S16R_KSTRING,     /* A pointer to const char*/
        S16R_KBOOL,       /* A boolptr_t */
        S16R_KINT,        /* An intptr_t */
        S16R_KNVLIST,     /* A pointer to an nvlist */
        S16R_KSTRUCT,     /* A pointer to a struct */
        S16R_KLIST,       /* An S16 list */
        S16R_KDESCRIPTOR, /* Unix rights - but cast into fdptr_t!! */
        S16R_KMAX
    } S16NVRPCTypeKind;

    typedef struct S16NVRPCType
    {
        S16NVRPCTypeKind kind;
        union {
            /* If kind is STRUCT, this points to the struct description. */
            struct S16NVRPCStruct * sdesc;
            /* If kind is LIST, this points to the type of its elements. */
            struct S16NVRPCType * ltype;
        };
    } S16NVRPCType;

    typedef struct
    {
        const char * name; /* Name */
        S16NVRPCType type; /* Type descriptor */
        size_t off;        /* Offset of field into corresponding struct */
    } S16NVRPCField;

    typedef struct S16NVRPCStruct
    {
        size_t len;             /* Total size of struct */
        S16NVRPCField fields[]; /* Field descriptions */
    } S16NVRPCStruct;

    typedef struct S16NVRPCMessageParameter
    {
        const char * name;
        S16NVRPCType type;
    } S16NVRPCMessageParameter;

    typedef struct S16NVRPCMessageSignature
    {
        const char * name; /* Message selector */
        bool raw; /* Arguments and return value passed as plain nvlist */
        S16NVRPCType rtype;              /* Return type of message */
        size_t nargs;                    /* Number of arguments */
        S16NVRPCMessageParameter args[]; /* Argument signatures */
    } S16NVRPCMessageSignature;

    /* S16 RPC Error code */
    typedef enum
    {
        /* The nvlist sent is not a valid Request object. */
        kS16NVRPCErrorInvalidRequest = -32600,
        /* The method does not exist / is not available. */
        kS16NVRPCErrorNoSuchMethod = -32601,
        /* Invalid method parameter(s). */
        kS16NVRPCErrorInvalidParams = -32602,
        /* Internal NVList-RPC error. */
        kS16NVRPCErrorInternalError = -32603,
        /*
         * 32000 to -32099	Server error	Reserved for implementation-defined
         * server-errors.
         */
    } S16NVRPCErrorCode;

    typedef struct
    {
        S16NVRPCErrorCode code; /* Error code */
        char * message;         /* Error message */
        size_t data_len;        /* Length of any auxiliary data */
        void * data;            /* Pointer to auxiliary data */
    } S16NVRPCError;

    typedef struct S16NVRPCCallContext
    {
        const char * method;
        S16NVRPCError err;
        nvlist_t * result;
        void * extra;
    } S16NVRPCCallContext;

    typedef struct S16NVRPCServer S16NVRPCServer;

    typedef void * (*S16NVRPCImplementationFn) (S16NVRPCCallContext *, ...);

    void serialise (nvlist_t * nvl, const char * name, void ** src,
                    S16NVRPCType * type);
    nvlist_t * S16NVRPCStructSerialise (void * src, S16NVRPCStruct * desc);

    /*
     * Deserialisation routines. They return 0 if they succeed.
     */
    int S16NVRPCStructDeserialise (nvlist_t * nvl, S16NVRPCStruct * desc,
                                   void ** dest);
    int S16NVRPCMemberDeserialise (nvlist_t * nvl, const char * name,
                                   S16NVRPCType * type, void ** dest);
    int S16NVRPCMessageSignatureDeserialiseArguments (
        nvlist_t * nvl, S16NVRPCMessageSignature * desc, void ** dest);

    /*
     * Destruction routines. Programmatically destroy based on descriptions.
     */
    void
    S16NVRPCMessageSignatureDestroyArguments (void ** src,
                                              S16NVRPCMessageSignature * desc);

    ucl_object_t * S16NVRPCNVListToUCL (const nvlist_t * nvl);

    /*
     * Destroy an S16NVRPC error.
     */
    void S16NVRPCErrorDestroy (S16NVRPCError * err);

    /*
     * Create a new NVRPC server.
     */
    S16NVRPCServer * S16NVRPCServerNew (void * extra);

    /*
     * Registers a method with the server.
     */
    void S16NVRPCServerRegisterMethod (S16NVRPCServer * srv,
                                       S16NVRPCMessageSignature * sig,
                                       S16NVRPCImplementationFn fun);

    /*
     * To be called when data is ready for reading from [one of] the file
     * descriptors on which you wish this NVRPC server to respond to calls
     * from.
     */
    void S16NVRPCServerReceiveFromFileDescriptor (S16NVRPCServer * server,
                                                  int fd);

    /*
     * Synchronous API
     */

    /*
     * Makes a synchronous call. Returns either an NVList in which key "result"
     * has the value of the result, or NULL and sets error to the error details
     * otherwise.
     */
    S16NVRPCError * S16NVRPCClientCallRaw (int fd, nvlist_t ** result,
                                           const char * methodName,
                                           nvlist_t * params);

    S16NVRPCError *
    S16NVRPCClientCallInternal (int fd, void ** result, size_t nparams,
                                S16NVRPCMessageSignature * signature, ...);

/* Makes an asynchronous call to  the given method on the client reached on the
 * On receiving reply, callback is called.
 * @param Descriptor on which to send.
 * @param Pointer to variable where result will be stored..
 * @param Message signature.
 * */
#define S16NVRPCClientCall(fd, result, ...)                                    \
    S16NVRPCClientCallInternal (                                               \
        fd, result, GET_ARG_COUNT (__VA_ARGS__), ##__VA_ARGS__)

    /*
     * Asynchronous API
     */

    typedef struct S16NVRPCAsynchronousCall
    {
        int id;
        nvlist_t * result;
    } S16NVRPCAsyncCall;

    S16ListType (S16NVRPCAsyncCall, S16NVRPCAsyncCall *);

    /*
     * This must be kept in order to do asynchronous calls.
     */
    typedef struct S16NVRPCAsyncContext
    {
        S16List (S16NVRPCAsyncCall) calls;
    } S16NVRPCAsyncContext;

    typedef void * (*S16NVRPCReplyReceivedCallback) (S16NVRPCCallContext *,
                                                     ...);

    S16NVRPCAsyncCall *
    S16NVRPCClientCallAsyncInternal (S16NVRPCAsyncContext * asyncContext,
                                     int fd, size_t nparams,
                                     S16NVRPCMessageSignature * signature, ...);

/* Makes an asynchronous call to  the given method on the client reached on the
 * On receiving reply, callback is called.
 * @param Asynchronous context.
 * @param Descriptor on which to send.
 * @param Message signature.
 * */
#define S16NVRPCClientCallAsync(asyncContext, fd, ...)                         \
    S16NVRPCClientCallAsyncInternal (                                          \
        asyncContext, fd, GET_ARG_COUNT (__VA_ARGS__), ##__VA_ARGS__)

    void testIt ();

#ifdef __cplusplus
}
#endif

#endif
