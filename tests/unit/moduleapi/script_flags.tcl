set testmodule [file normalize tests/modules/script_flags.so]
set test_script "#!lua
redis.call('set','x',1)
return 1"

proc createScript {flag} {
    return "#!lua flags=${flag}
redis.call('set','x',1)
return 1"
}

start_server {tags {"modules script_flags"}} {
    r module load $testmodule

    test {test module check script flags body} {
        assert_equal [r script_flags.get_flags_body ${test_script}] 0
        assert_equal [r script_flags.get_flags_body [createScript "no-writes"]] 1
        assert_equal [r script_flags.get_flags_body [createScript "allow-oom"]] 2
        assert_equal [r script_flags.get_flags_body [createScript "allow-stale"]] 4
        assert_equal [r script_flags.get_flags_body [createScript "no-cluster"]] 8
        assert_equal [r script_flags.get_flags_body [createScript "allow-cross-slot-keys"]] 32
        assert_equal [r script_flags.get_flags_body [createScript "no-writes,allow-oom"]] 3
    }

    test {test module check script flags sha} {
        set sha [r script load $test_script]
        assert_equal [r script_flags.get_flags_sha $sha] 0

        set sha [r script load [createScript "no-writes"]]
        assert_equal [r script_flags.get_flags_sha $sha] 1

        set sha [r script load [createScript "allow-oom"]]
        assert_equal [r script_flags.get_flags_sha $sha] 2

        set sha [r script load [createScript "allow-stale"]]
        assert_equal [r script_flags.get_flags_sha $sha] 4

        set sha [r script load [createScript "no-cluster"]]
        assert_equal [r script_flags.get_flags_sha $sha] 8

        set sha [r script load [createScript "allow-cross-slot-keys"]]
        assert_equal [r script_flags.get_flags_sha $sha] 32

        set sha [r script load [createScript "no-writes,allow-oom"]]
        assert_equal [r script_flags.get_flags_sha $sha] 3
    }
}
