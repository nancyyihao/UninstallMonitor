

#ifndef __FIND_PID_BY_NAME_H
#define __FIND_PID_BY_NAME_H

#ifdef __cplusplus
extern "C" {
#endif

#define PATH_MAX 1024
#define READ_BUF_SIZE 2048

int find_pid_by_name_status( char* pidName,int* foundpid);

int find_pid_by_name( char* ProcName, int* foundpid);

#ifdef __cplusplus
}
#endif

#endif /* __FIND_PID_BY_NAME_H */
