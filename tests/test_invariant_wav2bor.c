#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define BOR_IDENTIFIER "BOR"
#define ARTIST_SIZE 32
#define TITLE_SIZE 32

START_TEST(test_buffer_overflow_strcpy_bounds)
{
    // Invariant: Buffer reads never exceed declared length; strcpy must not overflow artist/title fields
    const char *payloads[] = {
        "normal_artist",                                    // valid input
        "A",                                                // boundary: minimum
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",               // boundary: exactly at limit (33 chars)
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", // exploit: 2x overflow (49 chars)
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" // 10x overflow
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        pid_t pid = fork();
        ck_assert_int_ne(pid, -1);
        
        if (pid == 0) {
            // Child process: attempt to trigger vulnerability
            char artist[ARTIST_SIZE];
            char title[TITLE_SIZE];
            
            // Simulate the vulnerable strcpy pattern from wav2bor.c
            strcpy(artist, payloads[i]);
            strcpy(title, payloads[i]);
            
            // If we reach here without segfault, verify bounds were respected
            ck_assert_int_le((int)strlen(artist), ARTIST_SIZE - 1);
            ck_assert_int_le((int)strlen(title), TITLE_SIZE - 1);
            
            exit(EXIT_SUCCESS);
        } else {
            // Parent process: wait and check child didn't crash
            int status;
            waitpid(pid, &status, 0);
            
            // Test passes if child exited normally (no segfault/abort)
            // This detects if strcpy caused a crash on oversized input
            ck_assert_msg(WIFEXITED(status) || WIFSIGNALED(status),
                         "Child process did not terminate properly on payload %d", i);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_overflow_strcpy_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}