set testmodule [file normalize tests/modules/script_flags.so]
set test_script "#!lua flags=allow-oom
redis.call('set','x',1)
return 1"

start_server {tags {"modules script_flags"}} {
    r module load $testmodule

    test {test module check script flags body} {
        assert_equal [r script_flags.get_flags_body "$test_script"] 2
    }

    test {test module check script flags sha} {
        set sha [r script load $test_script]
        assert_equal [r script_flags.get_flags_sha $sha] 2
    }
}
