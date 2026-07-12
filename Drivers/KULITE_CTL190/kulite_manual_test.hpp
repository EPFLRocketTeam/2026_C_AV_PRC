#ifndef CTL190_MANUAL_TEST_H
#define CTL190_MANUAL_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

int manual_test_ctl190(void);

int ctl190_test_reset(void);

int ctl190_test_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* CTL190_MANUAL_TEST_H */
