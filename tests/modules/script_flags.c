#include "../../src/redismodule.h"
#include <stdio.h>

int get_flags_body(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }

    uint64_t flags;
    RedisModuleString *err = RedisModule_GetScriptBodyFlags(ctx, argv[1], &flags);
    if (err != NULL) {
        size_t len;
        const char * str = RedisModule_StringPtrLen(err, &len);
        RedisModule_ReplyWithError(ctx, str);
        RedisModule_FreeString(ctx, err);

        return REDISMODULE_OK;
    }

    RedisModule_ReplyWithLongLong(ctx, flags);
    return REDISMODULE_OK;
}

int get_flags_sha(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc != 2) {
        return RedisModule_WrongArity(ctx);
    }

    uint64_t flags;
    RedisModuleString *err = RedisModule_GetScriptSHAFlags(ctx, argv[1], &flags);
    if (err != NULL) {
        size_t len;
        const char * str = RedisModule_StringPtrLen(err, &len);
        RedisModule_ReplyWithError(ctx, str);
        RedisModule_FreeString(ctx, err);

        return REDISMODULE_OK;
    }

    RedisModule_ReplyWithLongLong(ctx, flags);
    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    if (RedisModule_Init(ctx,"script_flags",1,REDISMODULE_APIVER_1)== REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"script_flags.get_flags_body", get_flags_body,"",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx,"script_flags.get_flags_sha", get_flags_sha,"",0,0,0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}
