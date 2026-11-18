#ifndef LOGGER_H
#define LOGGER_H

int logger_init(void);
void logger_log(const char *message);
int logger_shutdown(void);     /* returns number of events written to the log */
int logger_demo(void);         /* default repeatable benchmark (3 producers x 5 events) */
int logger_custom_run(void);   /* interactive: enter producer threads and event counts */
int logger_submit_event(void); /* interactive: enter custom message to log */

#endif
