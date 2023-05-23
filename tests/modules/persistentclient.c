
#include "redismodule.h"

#include <strings.h>
#include <string.h>

RedisModuleClient *client = NULL;
RedisModuleUser *ctx_user = NULL;
RedisModuleUser *client_user = NULL;

int mc_create(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (client == NULL) {
        client = RedisModule_CreateModuleClient(ctx);
    }

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int mc_reset_users(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (ctx_user != NULL) {
        RedisModule_FreeModuleUser(ctx_user);
    }

    ctx_user = RedisModule_CreateModuleUser("context-user");

    if (client_user != NULL) {
        RedisModule_FreeModuleUser(client_user);
    }

    client_user = RedisModule_CreateModuleUser("client-user");

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int mc_free_users(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (ctx_user != NULL) {
        RedisModule_FreeModuleUser(ctx_user);
    }
    ctx_user = NULL;

    if (client_user != NULL) {
        RedisModule_FreeModuleUser(client_user);
    }
    client_user = NULL;

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;

}

int mc_set_user_acl(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 3) {
        return RedisModule_WrongArity(ctx);
    }

    RedisModuleUser *user;

    size_t userstr_len;
    const char *userstr = RedisModule_StringPtrLen(argv[1], &userstr_len);

    if (strncasecmp("context-user", userstr, userstr_len) == 0) {
        user = ctx_user;
    } else if (strncasecmp("client-user", userstr, userstr_len) == 0) {
        user = client_user;
    } else {
        RedisModule_ReplyWithError(ctx, "unrecognized user type");;
        return REDISMODULE_OK;
    }

    size_t acl_len;
    const char *acl = RedisModule_StringPtrLen(argv[2], &acl_len);

    RedisModuleString *error;
    int ret = RedisModule_SetModuleUserACLString(ctx, user, acl, &error);
    if (ret) {
        size_t len;
        const char * e = RedisModule_StringPtrLen(error, &len);
        RedisModule_ReplyWithError(ctx, e);
        RedisModule_FreeString(NULL, error);
        return REDISMODULE_OK;
    }

    RedisModule_ReplyWithSimpleString(ctx, "OK");

    return REDISMODULE_OK;
}

int mc_delete(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (client != NULL) {
        RedisModule_FreeModuleClient(ctx, client);
        client = NULL;
    }

    RedisModule_ReplyWithSimpleString(ctx, "OK");
    return REDISMODULE_OK;
}

int do_exec(RedisModuleCtx *ctx, RedisModuleString **argv, int argc, int user_client, int user_context) {
    size_t cmdstr_len;
    const char *cmdstr = RedisModule_StringPtrLen(argv[1], &cmdstr_len);
    char flags[4] = {0};

    if (user_client) {
        printf("setting client user\n");
        RedisModule_SetClientUser(client, client_user);
    }

    if (user_context) {
        printf("setting context user\n");
        RedisModule_SetContextUser(ctx, ctx_user);
    }

    RedisModule_SetContextClient(ctx, client);

    if (user_client || user_context) {
        strncpy(flags, "CEv", sizeof(flags));
    } else {
        strncpy(flags, "Ev", sizeof(flags));
    }

    RedisModuleCallReply *reply = RedisModule_Call(ctx, cmdstr, flags, argv+2, argc-2);

    RedisModule_SetContextClient(ctx, NULL);

    if (user_client) {
        RedisModule_SetClientUser(client, NULL);
    }

    if (user_context) {
        RedisModule_SetContextUser(ctx, NULL);
    }

    RedisModule_ReplyWithCallReply(ctx, reply);
    RedisModule_FreeCallReply(reply);

    return REDISMODULE_OK;

}

int mc_exec(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (client == NULL) {
        RedisModule_ReplyWithError(ctx, "Client not already allocated");
        return REDISMODULE_OK;
    }

    if (argc <= 1) {
        RedisModule_ReplyWithError(ctx, "not enough arguments");
        return REDISMODULE_OK;
    }

    return do_exec(ctx, argv, argc, 0, 0);
}

int mc_exec_with_client_user(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (client == NULL) {
        RedisModule_ReplyWithError(ctx, "Client not already allocated");
        return REDISMODULE_OK;
    }

    if (argc <= 1) {
        RedisModule_ReplyWithError(ctx, "not enough arguments");
        return REDISMODULE_OK;
    }

    return do_exec(ctx, argv, argc, 1, 0);
}

int mc_exec_with_client_and_ctx_user(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (client == NULL) {
        RedisModule_ReplyWithError(ctx, "Client not already allocated");
        return REDISMODULE_OK;
    }

    if (argc <= 1) {
        RedisModule_ReplyWithError(ctx, "not enough arguments");
        return REDISMODULE_OK;
    }

    return do_exec(ctx, argv, argc, 1, 1);
}

int mc_getflags(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (client == NULL) {
        RedisModule_ReplyWithError(ctx, "Client not already allocated");
        return REDISMODULE_OK;
    }

    RedisModule_ReplyWithLongLong(ctx, RedisModule_GetClientFlags(client));
    return REDISMODULE_OK;
}

static void rm_call_async_send_reply(RedisModuleCtx *ctx, RedisModuleCallReply *reply) {
    RedisModule_ReplyWithCallReply(ctx, reply);
    RedisModule_FreeCallReply(reply);
}

static void rm_call_async_on_unblocked(RedisModuleCtx *ctx, RedisModuleCallReply *reply, void *private_data) {
    REDISMODULE_NOT_USED(ctx);

    RedisModuleBlockedClient *bc = private_data;
    RedisModuleCtx *bctx = RedisModule_GetThreadSafeContext(bc);
    rm_call_async_send_reply(bctx, reply);
    RedisModule_FreeThreadSafeContext(bctx);
    RedisModule_UnblockClient(bc, RedisModule_BlockClientGetPrivateData(bc));
}

static void do_rm_call_async_free_pd(RedisModuleCtx * ctx, void *pd) {
    REDISMODULE_NOT_USED(ctx);

    RedisModule_FreeCallReply(pd);
}

int mc_async(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < 2) {
        return RedisModule_WrongArity(ctx);
    }

    const char *cmd = RedisModule_StringPtrLen(argv[1], NULL);

    RedisModule_SetContextClient(ctx, client);
    RedisModuleCallReply* rep = RedisModule_Call(ctx, cmd, "KEv", argv + 2, argc - 2);
    RedisModule_SetContextClient(ctx, NULL);

    if(RedisModule_CallReplyType(rep) != REDISMODULE_REPLY_PROMISE) {
        rm_call_async_send_reply(ctx, rep);
    } else {
        RedisModuleBlockedClient *bc = RedisModule_BlockClient(ctx, NULL, NULL, do_rm_call_async_free_pd, 0);
        RedisModule_BlockClientSetPrivateData(bc, rep);
        RedisModule_CallReplyPromiseSetUnblockHandler(rep, rm_call_async_on_unblocked, bc);
    }

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (RedisModule_Init(ctx,"moduleclient",1,REDISMODULE_APIVER_1)== REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.create", mc_create,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.delete", mc_delete,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.exec", mc_exec,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.exec_with_client_user", mc_exec_with_client_user,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.exec_with_client_and_ctx_user", mc_exec_with_client_and_ctx_user,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"mc.getflags", mc_getflags,"write",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "mc.exec_async", mc_async, "write", 0, 0, 0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "mc.set_user_acl", mc_set_user_acl, "write", 0, 0, 0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "mc.reset_users", mc_reset_users, "write", 0, 0, 0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "mc.free_users", mc_free_users, "write", 0, 0, 0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}