#ifndef IPC_H
#define IPC_H

int ipc_demo(void);   /* default benchmark (job 101, burst 5) */
int ipc_custom(void); /* interactive: enter custom job and burst */
void ipc_cleanup(void); /* removes only this program's shm and semaphores */
int ipc_objects_remaining(void); /* how many of those objects still exist */

#endif
